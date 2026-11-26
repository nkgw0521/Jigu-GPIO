#requires -Version 5.1
[CmdletBinding()]
param(
    [string]$CubeIdePath = 'C:\ST\STM32CubeIDE_2.1.1\STM32CubeIDE'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$PSNativeCommandUseErrorActionPreference = $false

$repo = $PSScriptRoot
$headerPath = 'App/Inc/scpi.h'
$manifestPath = 'tests/release-binaries.json'
$sourceRoots = @('App', 'Boards', 'libscpi')
$boards = [ordered]@{
    'NUCLEO-F303K8'   = 'NUCLEO-F303K8'
    'NUCLEO-L073RZ'   = 'NUCLEO-L073RZ'
    'NUCLEO_L053R8'   = 'Jigu-GPIO_L053'
    'NUCLEO_L432KC'   = 'Jigu-GPIO_L432'
    'STM32L0538-DISCO' = 'Jigu-GPIO'
}
$binaryPaths = @(foreach ($board in $boards.Keys) {
    "Boards/$board/Release/$($boards[$board]).bin"
})
$releasePaths = @($headerPath) + $binaryPaths + @($manifestPath)
$utf8 = [Text.UTF8Encoding]::new($false)
$versionPattern = '(?m)(^#define[ \t]+JIGU_GPIO_FW_VERSION[ \t]+")([0-9]{2})-([0-9]{2})("[ \t]*\r?$)'

function Invoke-ReleaseGit {
    $previousEncoding = [Console]::OutputEncoding
    try {
        [Console]::OutputEncoding = $utf8
        & git -C $repo -c core.quotepath=false @args
        if ($LASTEXITCODE -ne 0) { throw "git $($args[0]) failed (exit $LASTEXITCODE)." }
    } finally {
        [Console]::OutputEncoding = $previousEncoding
    }
}

function Get-Sha256 {
    param([string]$Path, [switch]$AllowMissing)
    if ($AllowMissing -and -not (Test-Path -LiteralPath $Path)) { return $null }
    (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
}

function Get-SourceChanges {
    $paths = @(
        Invoke-ReleaseGit diff --name-only --no-renames HEAD -- @sourceRoots
        Invoke-ReleaseGit ls-files --others --exclude-standard -- @sourceRoots
    )
    foreach ($path in ($paths | Sort-Object -Unique)) {
        if ($path -in $binaryPaths) { continue }
        $file = Join-Path $repo $path
        $sha256 = Get-Sha256 $file -AllowMissing
        [ordered]@{ path = $path; sha256 = $sha256 }
    }
}

$lock = $null
$stage = $null
$completed = $false
try {
    $ide = Join-Path $CubeIdePath 'stm32cubeidec.exe'
    if (-not (Test-Path -LiteralPath $ide -PathType Leaf)) {
        throw "STM32CubeIDE was not found at $ide. Set -CubeIdePath to the directory containing stm32cubeidec.exe."
    }
    $ide = (Get-Item -LiteralPath $ide).FullName
    $plugins = Join-Path (Split-Path -Parent $ide) 'plugins'
    $toolchains = @(Get-ChildItem -LiteralPath $plugins -Directory -Filter 'com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.*' |
        ForEach-Object { Join-Path $_.FullName 'tools\bin' } |
        Where-Object {
            (Test-Path -LiteralPath (Join-Path $_ 'arm-none-eabi-gcc.exe')) -and
            (Test-Path -LiteralPath (Join-Path $_ 'arm-none-eabi-objcopy.exe'))
        })
    if ($toolchains.Count -ne 1) {
        throw "Expected one bundled STM32 GNU toolchain under $plugins. Found $($toolchains.Count)."
    }
    $gcc = Join-Path $toolchains[0] 'arm-none-eabi-gcc.exe'
    $objcopy = Join-Path $toolchains[0] 'arm-none-eabi-objcopy.exe'
    $compiler = & $gcc --version | Select-Object -First 1
    if ($LASTEXITCODE -ne 0) { throw 'Could not identify the STM32 GNU compiler.' }

    $baseCommit = Invoke-ReleaseGit rev-parse HEAD
    # Hold the lock on the repository so releases from different PCs serialize on the share.
    $lockPath = Join-Path $repo '.firmware-release.lock'
    try {
        $lock = [IO.FileStream]::new($lockPath, [IO.FileMode]::CreateNew, [IO.FileAccess]::Write,
            [IO.FileShare]::None, 4096, [IO.FileOptions]::DeleteOnClose)
    } catch [IO.IOException] {
        throw "Could not acquire the release lock at $lockPath. $($_.Exception.Message)"
    }

    $tempRoot = [IO.Path]::GetFullPath([IO.Path]::GetTempPath())
    $stage = Join-Path $tempRoot ('jigu-fw-' + [guid]::NewGuid().ToString('N').Substring(0, 12))
    $source = Join-Path $stage 'src'
    $backup = Join-Path $stage 'backup'
    New-Item -ItemType Directory -Path $source, $backup | Out-Null

    $snapshot = @{}
    foreach ($path in $releasePaths) {
        $saved = Join-Path $backup $path
        $original = Join-Path $repo $path
        if (Test-Path -LiteralPath $original -PathType Leaf) {
            New-Item -ItemType Directory -Path (Split-Path -Parent $saved) -Force | Out-Null
            Copy-Item -LiteralPath $original -Destination $saved
        }
        $snapshot[$path] = Get-Sha256 $saved -AllowMissing
    }
    $headerText = [IO.File]::ReadAllText((Join-Path $backup $headerPath))
    $versionMatches = [regex]::Matches($headerText, $versionPattern)
    if ($versionMatches.Count -ne 1) { throw "Expected one NN-NN firmware version in $headerPath." }
    $versionMatch = $versionMatches[0]
    $major = [int]$versionMatch.Groups[2].Value
    $minor = [int]$versionMatch.Groups[3].Value
    $oldVersion = '{0:D2}-{1:D2}' -f $major, $minor
    if ($minor -eq 99) {
        if ($major -eq 99) { throw 'Firmware version 99-99 has no successor in the NN-NN format.' }
        $major++
        $minor = 0
    } else {
        $minor++
    }
    $version = '{0:D2}-{1:D2}' -f $major, $minor
    Write-Host "Firmware release $oldVersion -> $version"

    $workingChanges = @(Get-SourceChanges)
    $archive = Join-Path $stage 'source.tar'
    Invoke-ReleaseGit archive --format=tar "--output=$archive" $baseCommit @sourceRoots
    & tar.exe -xf $archive -C $source
    if ($LASTEXITCODE -ne 0) { throw 'Could not extract the release source archive.' }

    # Build the working sources, including changes since the base commit.
    $overrides = @(foreach ($change in $workingChanges) {
        if ($change.path -eq $headerPath) { continue }
        $destination = Join-Path $source $change.path
        if ($null -eq $change.sha256) {
            if (Test-Path -LiteralPath $destination -PathType Leaf) {
                Remove-Item -LiteralPath $destination
            }
        } else {
            New-Item -ItemType Directory -Path (Split-Path -Parent $destination) -Force | Out-Null
            Copy-Item -LiteralPath (Join-Path $repo $change.path) -Destination $destination
            if ((Get-Sha256 $destination) -ne $change.sha256) {
                throw "Build input changed while copying $($change.path)."
            }
        }
        $change
    })
    $newDefinition = $versionMatch.Groups[1].Value + $version + $versionMatch.Groups[4].Value
    $newHeader = $headerText.Remove($versionMatch.Index, $versionMatch.Length).Insert($versionMatch.Index, $newDefinition)
    $buildHeader = Join-Path $source $headerPath
    [IO.File]::WriteAllText($buildHeader, $newHeader, $utf8)
    $overrides += [ordered]@{ path = $headerPath; sha256 = (Get-Sha256 $buildHeader) }

    $ideArguments = @('--launcher.suppressErrors', '-nosplash', '-application',
        'org.eclipse.cdt.managedbuilder.core.headlessbuild', '-data', (Join-Path $stage 'workspace'))
    foreach ($board in $boards.Keys) {
        $ideArguments += @('-import', (Join-Path $source "Boards/$board"))
    }
    # Require new BINs even if a tracked artifact exists in the source archive.
    foreach ($path in $binaryPaths) {
        $bin = Join-Path $source $path
        if (Test-Path -LiteralPath $bin -PathType Leaf) { Remove-Item -LiteralPath $bin }
    }
    $ideArguments += @('-cleanBuild', '.*/Release', '-no-indexer')
    $buildLog = Join-Path $stage 'build.log'
    Write-Host 'Building the five Release projects in STM32CubeIDE...'
    Push-Location -LiteralPath $stage
    $previousErrorAction = $ErrorActionPreference
    try {
        # Windows PowerShell treats native stderr as errors. Let CubeIDE finish and use its exit code.
        $ErrorActionPreference = 'Continue'
        & $ide @ideArguments *> $buildLog
        $buildExit = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $previousErrorAction
        Pop-Location
    }
    if ($buildExit -ne 0) { throw "CubeIDE failed (exit $buildExit). See $buildLog." }

    $log = Get-Content -LiteralPath $buildLog -Raw
    $entries = @(foreach ($path in $binaryPaths) {
        $board = ($path -split '/')[1]
        $project = $boards[$board]
        $buildPattern = '(?ms)^\S+ \*+ Build of configuration Release for project ' +
            [regex]::Escape($project) + ' \*+[^\r\n]*(?<output>(?:(?!^\S+ Build Finished\.).)*?)' +
            '^\S+ Build Finished\. 0 errors, [0-9]+ warnings?\.'
        $buildResult = [regex]::Match($log, $buildPattern)
        if (-not $buildResult.Success) { throw "Successful Release build not recorded for $board." }

        $bin = Join-Path $source $path
        $elf = [IO.Path]::ChangeExtension($bin, '.elf')
        $checkBin = Join-Path $stage "$project.objcopy.bin"
        & $objcopy -O binary $elf $checkBin
        if ($LASTEXITCODE -ne 0) { throw "ELF conversion failed for $board." }
        $sha256 = Get-Sha256 $bin
        if ((Get-Sha256 $checkBin) -ne $sha256) { throw "ELF and BIN differ for $board." }
        $image = [Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes($bin))
        if (-not $image.Contains($version + [char]0)) { throw "Firmware version $version is missing from the $board BIN." }

        $warning = if ($buildResult.Groups['output'].Value -match 'LOAD segment with RWX permissions') {
            'RWX LOAD segment'
        } else { $null }
        $size = (Get-Item -LiteralPath $bin).Length
        Write-Host "Verified $board ($size bytes)"
        [ordered]@{
            board = $board
            path = $path
            bytes = $size
            sha256 = $sha256
            full_release_rebuild = 'passed'
            elf_binary_comparison = 'identical'
            linker_warning = $warning
        }
    })
    $manifest = [ordered]@{
        date = Get-Date -Format 'yyyy-MM-dd'
        firmware_version = $version
        source_base_commit = $baseCommit
        source_overrides = $overrides
        compiler = $compiler
        build = 'STM32CubeIDE headless clean Release build for all five boards. Auto-generated BINs match GNU objcopy -O binary output from each ELF.'
        binaries = $entries
    }
    $json = ($manifest | ConvertTo-Json -Depth 6).Replace([Environment]::NewLine, [string][char]10) + [char]10
    $buildManifest = Join-Path $source $manifestPath
    New-Item -ItemType Directory -Path (Split-Path -Parent $buildManifest) -Force | Out-Null
    [IO.File]::WriteAllText($buildManifest, $json, $utf8)

    $currentChanges = @(Get-SourceChanges)
    if ((Invoke-ReleaseGit rev-parse HEAD) -ne $baseCommit -or
        (ConvertTo-Json -InputObject $currentChanges -Compress) -cne (ConvertTo-Json -InputObject $workingChanges -Compress)) {
        throw 'Build inputs changed during the release. Run the release again.'
    }
    foreach ($path in $releasePaths) {
        if ((Get-Sha256 (Join-Path $repo $path) -AllowMissing) -ne $snapshot[$path]) {
            throw "Release file changed during the build: $path"
        }
    }

    $attempted = @()
    try {
        foreach ($path in $releasePaths) {
            $from = Join-Path $source $path
            $to = Join-Path $repo $path
            New-Item -ItemType Directory -Path (Split-Path -Parent $to) -Force | Out-Null
            $attempted += $path
            Copy-Item -LiteralPath $from -Destination $to
            if ((Get-Sha256 $to) -ne (Get-Sha256 $from)) { throw "Write verification failed for $path." }
        }
    } catch {
        foreach ($path in $attempted) {
            try {
                $target = Join-Path $repo $path
                if ($null -ne $snapshot[$path]) {
                    Copy-Item -LiteralPath (Join-Path $backup $path) -Destination $target
                } elseif (Test-Path -LiteralPath $target -PathType Leaf) {
                    Remove-Item -LiteralPath $target
                }
                if ((Get-Sha256 $target -AllowMissing) -ne $snapshot[$path]) {
                    throw "Restore verification failed for $path."
                }
            } catch {
                Write-Warning "Could not restore $path. The original is in $backup."
            }
        }
        throw
    }

    $completed = $true
    Write-Host "Firmware $version prepared. Review the Git diff before committing the release."
} finally {
    if ($null -ne $lock) { $lock.Dispose() }
    if ($completed -and $null -ne $stage) {
        $resolved = (Resolve-Path -LiteralPath $stage).ProviderPath
        if ($resolved -eq $stage -and (Split-Path -Parent $resolved) -eq $tempRoot.TrimEnd('\')) {
            Remove-Item -LiteralPath $resolved -Recurse -ErrorAction SilentlyContinue
        }
    } elseif ($null -ne $stage) {
        Write-Warning "Release files and logs are in $stage."
    }
}
