# Stage 14 Acceptance Test — MCP Resources Knowledge Layer
# Tests: MCP protocol resources/list + resources/read (via Python subprocess)
#        TCP-level regression (PowerShell direct TCP)

$bridgeHost = "127.0.0.1"
$port = 9876
$testDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$outFile = Join-Path $testDir "stage14_test_result.txt"

function Send-Request($action, $payload) {
    $req = @{ id = (Get-Random -Maximum 99999).ToString(); action = $action; payload = $payload }
    $json = (ConvertTo-Json -Compress $req) + "`n"
    try {
        $s = New-Object System.Net.Sockets.TcpClient($bridgeHost, $port)
        $stream = $s.GetStream()
        $writer = New-Object System.IO.StreamWriter($stream)
        $writer.Write($json)
        $writer.Flush()
        $reader = New-Object System.IO.StreamReader($stream)
        $line = $reader.ReadLine()
        $s.Close()
        return ConvertFrom-Json $line
    } catch {
        return @{ ok = $false; error = @{ code = "CONNECT_FAILED"; message = $_.Exception.Message } }
    }
}

$log = @()
$failCount = 0
$passCount = 0

function Assert($label, $condition, $detail) {
    if ($condition) {
        $script:passCount++
        $script:log += "PASS: $label"
    } else {
        $script:failCount++
        $script:log += "FAIL: $label -- $detail"
    }
}

Start-Sleep -Milliseconds 500

# ============================================
# Part 1: MCP Protocol Resources Test (via Python subprocess)
# ============================================
$log += "===== Part 1: MCP Protocol Resources Test ====="

$mcpTestScript = Join-Path $testDir "test_resources_mcp.py"
$pyExe = (Get-Command python -ErrorAction SilentlyContinue).Source
if (-not $pyExe) { $pyExe = "python" }

$log += "Using Python: $pyExe"
$log += "Test script: $mcpTestScript"

try {
    $mcpOutput = & $pyExe $mcpTestScript 2>&1
    $exitCode = $LASTEXITCODE
    $log += $mcpOutput
    
    # Parse PASS/FAIL lines from MCP test output
    $mcpLines = $mcpOutput | Where-Object { $_ -match '^\s*(PASS|FAIL):' }
    foreach ($line in $mcpLines) {
        if ($line -match '^\s*PASS:') {
            $script:passCount++
            $script:log += "MCP PASS: $($line -replace '^\s*PASS:\s*', '')"
        } elseif ($line -match '^\s*FAIL:') {
            $script:failCount++
            $script:log += "MCP FAIL: $($line -replace '^\s*FAIL:\s*', '')"
        }
    }
    
    if ($exitCode -eq 0) {
        Assert "1.0 MCP protocol test exit code" $true "exit=0"
    } else {
        Assert "1.0 MCP protocol test exit code" $false "exit=$exitCode"
    }
} catch {
    $log += "EXCEPTION running MCP test: $_"
    Assert "1.0 MCP protocol test ran" $false "$_"
}
$log += ""

# ============================================
# Part 2: Live Resource Data Sources (TCP)
# ============================================
$log += "===== Part 2: Live Resource Data Sources ====="

Start-Sleep -Milliseconds 300
$r = Send-Request "get_mcp_config" @{}
if ($r.ok) {
    Assert "2.1 get_mcp_config ok" $true "ok"
    Assert "2.2 has actions" ($r.result.PSObject.Properties.Name -contains "actions") "missing"
    Assert "2.3 has mode" ($r.result.PSObject.Properties.Name -contains "mode") "missing"
    Assert "2.4 has token_enabled" ($r.result.PSObject.Properties.Name -contains "token_enabled") "missing"
} else {
    Assert "2.x get_mcp_config" $false "error=$($r.error.code)"
}
$log += ""

Start-Sleep -Milliseconds 300
$r = Send-Request "get_bridge_runtime_status" @{}
if ($r.ok) {
    Assert "2.5 get_bridge_runtime_status ok" $true "ok"
    Assert "2.6 has server_status" ($r.result.PSObject.Properties.Name -contains "server_status") "missing"
    Assert "2.7 has client_connected" ($r.result.PSObject.Properties.Name -contains "client_connected") "missing"
    Assert "2.8 has transport_mode" ($r.result.PSObject.Properties.Name -contains "transport_mode") "missing"
} else {
    Assert "2.x get_bridge_runtime_status" $false "error=$($r.error.code)"
}
$log += ""

# ============================================
# Part 3: Tools Regression (TCP)
# ============================================
$log += "===== Part 3: Tools Regression ====="

Start-Sleep -Milliseconds 300
$r = Send-Request "ping"
Assert "3.1 ping regression" ($r.ok -eq $true) "ok=$($r.ok)"
$log += ""

Start-Sleep -Milliseconds 300
function Send-RawRequest($raw) {
    try {
        $s = New-Object System.Net.Sockets.TcpClient($bridgeHost, $port)
        $stream = $s.GetStream()
        $writer = New-Object System.IO.StreamWriter($stream)
        $writer.Write($raw + "`n")
        $writer.Flush()
        $reader = New-Object System.IO.StreamReader($stream)
        $line = $reader.ReadLine()
        $s.Close()
        return ConvertFrom-Json $line
    } catch { return $null }
}
$r = Send-RawRequest "not json"
Assert "3.2 PARSE_ERROR regression" (($r -ne $null) -and ($r.error.code -eq "PARSE_ERROR")) "code=$($r.error.code)"
$log += ""

Start-Sleep -Milliseconds 300
$r = Send-Request "get_mcp_config" @{}
Assert "3.3 get_mcp_config still works" ($r.ok -eq $true) "ok=$($r.ok)"
$log += ""

# ============================================
# Summary
# ============================================
$log += "===== SUMMARY ====="
$log += "Passed: $passCount"
$log += "Failed: $failCount"
$log += "Total:  $($passCount + $failCount)"
if ($failCount -eq 0) {
    $log += "RESULT: ALL TESTS PASSED"
} else {
    $log += "RESULT: $failCount TEST(S) FAILED"
}

$log | Out-File -FilePath $outFile -Encoding utf8
Write-Host "DONE - Results written to $outFile"
Write-Host "Passed: $passCount, Failed: $failCount"
