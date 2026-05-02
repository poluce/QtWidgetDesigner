param(
    [Parameter(Mandatory = $true)]
    [string]$Program,
    [string[]]$Arguments = @(),
    [string]$WorkingDirectory = "",
    [string]$BridgeUrl = "ws://127.0.0.1:49555",
    [int]$StartupTimeoutMs = 10000
)

$ErrorActionPreference = "Stop"

Add-Type -AssemblyName System.Net.WebSockets

$resolvedProgram = [System.IO.Path]::GetFullPath($Program)
if (-not (Test-Path $resolvedProgram)) {
    throw "Program not found: $resolvedProgram"
}

$resolvedWorkingDirectory = if ([string]::IsNullOrWhiteSpace($WorkingDirectory)) {
    Split-Path $resolvedProgram
} else {
    [System.IO.Path]::GetFullPath($WorkingDirectory)
}

$proc = Start-Process -FilePath $resolvedProgram -ArgumentList $Arguments -WorkingDirectory $resolvedWorkingDirectory -PassThru

try {
    $deadline = (Get-Date).AddMilliseconds($StartupTimeoutMs)
    do {
        $ws = [System.Net.WebSockets.ClientWebSocket]::new()
        try {
            $ws.ConnectAsync([Uri]$BridgeUrl, [System.Threading.CancellationToken]::None).GetAwaiter().GetResult()
            $payload = @{ id = 'verify-runtime-bridge'; command = 'ping' } | ConvertTo-Json -Compress
            $bytes = [System.Text.Encoding]::UTF8.GetBytes($payload)
            $seg = [System.ArraySegment[byte]]::new($bytes)
            $ws.SendAsync($seg, [System.Net.WebSockets.WebSocketMessageType]::Text, $true, [System.Threading.CancellationToken]::None).GetAwaiter().GetResult()

            $buffer = New-Object byte[] 4096
            $recv = [System.ArraySegment[byte]]::new($buffer)
            $result = $ws.ReceiveAsync($recv, [System.Threading.CancellationToken]::None).GetAwaiter().GetResult()
            $text = [System.Text.Encoding]::UTF8.GetString($buffer, 0, $result.Count)
            $response = $text | ConvertFrom-Json

            if ($response.ok -eq $true -and $response.result.message -eq 'pong') {
                Write-Host "Bridge is ready at $BridgeUrl" -ForegroundColor Green
                return
            }
        }
        catch {
        }
        finally {
            $ws.Dispose()
        }

        if ($proc.HasExited) {
            throw "Target process exited before bridge became ready. ExitCode=$($proc.ExitCode)"
        }

        Start-Sleep -Milliseconds 200
    } while ((Get-Date) -lt $deadline)

    throw "Timed out waiting for bridge at $BridgeUrl"
}
finally {
    if (-not $proc.HasExited) {
        Stop-Process -Id $proc.Id
    }
}
