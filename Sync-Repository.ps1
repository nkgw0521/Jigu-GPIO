#requires -Version 5.1
<#
.SYNOPSIS
.gitを保持してリポジトリの全内容を同期し、SHA-256で検証します。
.DESCRIPTION
既定では、このスクリプトのあるフォルダーから、同じ親フォルダーの
__Jigu-GPIOへ同期します。同期先だけに存在するファイル・フォルダーは削除します。
Gitのコミット・プッシュは行いません。ルート直下の.gitは双方とも保持します。
.PARAMETER CheckOnly
書き込みを行わず、ファイル名・ディレクトリ構成・SHA-256を比較します。
.EXAMPLE
.\Sync-Repository.ps1
.EXAMPLE
.\Sync-Repository.ps1 -CheckOnly
.EXAMPLE
.\Sync-Repository.ps1 -WhatIf
.NOTES
終了コード: 0=同期・検証成功またはWhatIf、1=差分あり・失敗。
同期中は、両フォルダーでの編集やビルドを停止してください。
#>
[CmdletBinding(SupportsShouldProcess = $true, ConfirmImpact = 'Medium')]
param(
    [string]$SourcePath,
    [string]$DestinationPath,
    [switch]$CheckOnly
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Resolve-SyncRoot {
    param([string]$Path)
    if ([string]::IsNullOrWhiteSpace($Path)) { throw 'フォルダーを指定してください。' }
    $resolved = Resolve-Path -LiteralPath $Path -ErrorAction Stop
    if ($resolved.Provider.Name -ne 'FileSystem') { throw "ファイルシステムのパスが必要です: $Path" }
    $fullPath = $resolved.ProviderPath.TrimEnd('\', '/')
    # X:とUNCを同じ場所として比較できるよう、ネットワークドライブをUNCへ変換します。
    if ($fullPath -match '^([A-Za-z]):') {
        $drive = Get-PSDrive -Name $Matches[1] -PSProvider FileSystem
        if ($drive.DisplayRoot -and $drive.DisplayRoot.StartsWith('\\')) {
            $fullPath = $drive.DisplayRoot.TrimEnd('\') + $fullPath.Substring(2)
        }
    }
    if (-not [System.IO.Directory]::Exists($fullPath)) { throw "既存のフォルダーが必要です: $fullPath" }
    $fullPath = (Get-Item -LiteralPath $fullPath -Force).FullName.TrimEnd('\', '/')
    if ($fullPath -match '^[A-Za-z]:$' -or $fullPath -match '^\\\\[^\\]+\\[^\\]+$') {
        throw "ドライブまたは共有のルートは同期対象にできません: $fullPath"
    }
    if (-not (Test-Path -LiteralPath (Join-Path $fullPath '.git'))) {
        throw "リポジトリ直下を指定してください（.gitがありません）: $fullPath"
    }
    if ((Get-Item -LiteralPath $fullPath -Force).Attributes -band [System.IO.FileAttributes]::ReparsePoint) {
        throw "リンク先フォルダーは同期対象にできません: $fullPath"
    }
    return $fullPath
}

function Get-SyncEntries {
    param([string]$RootPath)
    $pending = New-Object 'System.Collections.Generic.Queue[string]'
    $pending.Enqueue($RootPath)
    while ($pending.Count -gt 0) {
        $directory = $pending.Dequeue()
        foreach ($item in Get-ChildItem -LiteralPath $directory -Force -ErrorAction Stop) {
            if ($directory -ieq $RootPath -and $item.Name -ieq '.git') { continue }
            if ($item.Attributes -band [System.IO.FileAttributes]::ReparsePoint) {
                throw "ジャンクション・シンボリックリンクは同期対象にできません: $($item.FullName)"
            }
            $item
            if ($item.PSIsContainer) { $pending.Enqueue($item.FullName) }
        }
    }
}

function Get-SyncManifest {
    param([string]$RootPath)
    $manifest = @{}
    foreach ($item in Get-SyncEntries -RootPath $RootPath) {
        $relativePath = $item.FullName.Substring($RootPath.Length).TrimStart('\')
        if ($item.PSIsContainer) {
            $manifest[$relativePath + '\'] = 'DIRECTORY'
        } else {
            $manifest[$relativePath] = (Get-FileHash -LiteralPath $item.FullName -Algorithm SHA256 -ErrorAction Stop).Hash
        }
    }
    return ,$manifest
}

try {
    $scriptDirectory = [System.IO.Path]::GetDirectoryName($MyInvocation.MyCommand.Path)
    if ([string]::IsNullOrWhiteSpace($SourcePath)) { $SourcePath = $scriptDirectory }
    if ([string]::IsNullOrWhiteSpace($DestinationPath)) {
        $DestinationPath = Join-Path (Split-Path -Parent $scriptDirectory) '__Jigu-GPIO'
    }
    $source = Resolve-SyncRoot -Path $SourcePath
    $destination = Resolve-SyncRoot -Path $DestinationPath
    if ($source -ieq $destination -or
        $source.StartsWith($destination + '\', [System.StringComparison]::OrdinalIgnoreCase) -or
        $destination.StartsWith($source + '\', [System.StringComparison]::OrdinalIgnoreCase)) {
        throw '同期元と同期先は、異なるフォルダーで、互いに包含しないパスにしてください。'
    }
    Write-Host "同期元: $source"
    Write-Host "同期先: $destination"

    if (-not $CheckOnly) {
        # 削除を伴う操作の前に、両方の範囲とリンクの有無を確認します。
        $sourceEntries = @(Get-SyncEntries -RootPath $source)
        $null = @(Get-SyncEntries -RootPath $destination)
        if (@($sourceEntries | Where-Object { -not $_.PSIsContainer }).Count -eq 0) {
            throw '同期元に.git以外のファイルがないため、同期を中止します。'
        }
        $null = Get-Command robocopy.exe -ErrorAction Stop
        if (-not $PSCmdlet.ShouldProcess($destination, 'ルートの.gitを保持し、同期先だけの内容を削除して完全同期')) {
            return
        }
        # /IS /IT /IMにより、サイズ・更新日時が同じファイルも内容を確実に揃えます。
        $copyArguments = @(
            $source, $destination, '/MIR', '/IS', '/IT', '/IM',
            '/XD', (Join-Path $source '.git'), (Join-Path $destination '.git'),
            '/XF', (Join-Path $source '.git'), (Join-Path $destination '.git'),
            '/XJ', '/COPY:DAT', '/DCOPY:DAT', '/R:0', '/W:0',
            '/NP', '/NFL', '/NDL', '/NJH', '/NJS'
        )
        $copyOutput = & robocopy.exe @copyArguments 2>&1
        $copyExitCode = $LASTEXITCODE
        if ($copyExitCode -ge 8) {
            $copyOutput | ForEach-Object { Write-Host $_ }
            throw "robocopyが失敗しました（終了コード: $copyExitCode）。部分的に同期されている可能性があります。"
        }
    }

    Write-Host 'ファイル・ディレクトリ構成とSHA-256を比較しています...'
    $sourceManifest = Get-SyncManifest -RootPath $source
    $destinationManifest = Get-SyncManifest -RootPath $destination
    $differences = New-Object 'System.Collections.Generic.List[string]'
    foreach ($key in $sourceManifest.Keys) {
        if (-not $destinationManifest.ContainsKey($key)) {
            $differences.Add("同期先に不足: $key")
        } elseif ($sourceManifest[$key] -cne $destinationManifest[$key]) {
            $differences.Add("内容が不一致: $key")
        }
    }
    foreach ($key in $destinationManifest.Keys) {
        if (-not $sourceManifest.ContainsKey($key)) { $differences.Add("同期先だけに存在: $key") }
    }
    if ($differences.Count -gt 0) {
        $differences | Sort-Object | ForEach-Object { Write-Host $_ }
        throw "差分が$($differences.Count)件あります。"
    }
    $fileCount = @($sourceManifest.Values | Where-Object { $_ -ne 'DIRECTORY' }).Count
    $directoryCount = $sourceManifest.Count - $fileCount
    Write-Host "確認完了: 差分0件（ファイル$fileCount 件、ディレクトリ$directoryCount 件）。ルートの.gitは除外。"
} catch {
    Write-Error -Message $_.Exception.Message -ErrorAction Continue
    exit 1
}
