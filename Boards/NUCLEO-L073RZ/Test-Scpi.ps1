[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidatePattern('^COM[0-9]+$')]
    [string]$PortName,
    [ValidateRange(100, 30000)]
    [int]$TimeoutMs = 3000
)

$ErrorActionPreference = 'Stop'
$port = [System.IO.Ports.SerialPort]::new(
    $PortName, 115200, [System.IO.Ports.Parity]::None, 8,
    [System.IO.Ports.StopBits]::One)
$port.Handshake = [System.IO.Ports.Handshake]::None
$port.DtrEnable = $true
$port.RtsEnable = $true
$port.ReadTimeout = $TimeoutMs
$port.WriteTimeout = $TimeoutMs
$port.NewLine = [string][char]10
$port.Encoding = [System.Text.Encoding]::ASCII

function Invoke-ScpiQuery {
    param([string]$Command)
    $deadline = [DateTime]::UtcNow.AddMilliseconds($TimeoutMs)
    $port.WriteLine($Command)
    while ([DateTime]::UtcNow -lt $deadline) {
        $port.ReadTimeout = [Math]::Max(1, [int]($deadline - [DateTime]::UtcNow).TotalMilliseconds)
        try {
            $line = $port.ReadLine().Trim()
        }
        catch [System.TimeoutException] {
            throw "No SCPI response for $Command on $PortName within $TimeoutMs ms (an echo alone is not a response)."
        }
        # The shared app echoes commands and may emit blank lines for CR/LF.
        if ($line.Length -eq 0 -or $line -eq $Command) { continue }
        return $line
    }
    throw "No SCPI response for $Command on $PortName"
}

try {
    $port.Open()
    $port.DiscardInBuffer()
    $idn = Invoke-ScpiQuery '*IDN?'
    if ($idn -notmatch '^MANUFACTURE,INSTR2013,') { throw "Unexpected identification: $idn" }
    [pscustomobject]@{ Command = '*IDN?'; Response = $idn }
    $opc = Invoke-ScpiQuery '*OPC?'
    if ($opc -ne '1') { throw "Unexpected *OPC? response: $opc" }
    [pscustomobject]@{ Command = '*OPC?'; Response = $opc }

    foreach ($command in @('GPIO:SEL?', 'PWM:FREQ?', 'PWM:WIDTH?', 'PWM:START?')) {
        $response = Invoke-ScpiQuery $command
        if ($response -notmatch '^[+]?[0-9]+$') { throw ('Invalid response for {0}: {1}' -f $command, $response) }
        [pscustomobject]@{ Command = $command; Response = $response }
    }
    $errorResponse = Invoke-ScpiQuery 'SYST:ERR?'
    if ($errorResponse -notmatch '^[+]?0,') { throw "SCPI error: $errorResponse" }
    [pscustomobject]@{ Command = 'SYST:ERR?'; Response = $errorResponse }
}
finally {
    if ($port.IsOpen) { $port.Close() }
    $port.Dispose()
}
