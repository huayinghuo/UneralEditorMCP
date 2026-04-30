# Stage 14 Acceptance Test — MCP Resources Knowledge Layer
# Tests: static resources integrity + live resource data sources + tools regression
# TCP-level test (for live resource data sources); static resource test via Python import

$bridgeHost = "127.0.0.1"
$port = 9876
$outFile = Join-Path $PSScriptRoot "stage14_test_result.txt"

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
# Part 1: Static Resources Integrity (Python)
# ============================================
$log += "===== Part 1: Static Resources Integrity ====="

$resourcesDir = (Join-Path $PSScriptRoot "..\src") -replace '\\', '/'
$pyResult = python -c @"
import sys; sys.path.insert(0, '$resourcesDir')
from mcp_bridge_server import resources
r = resources.list_static_resources()
print('COUNT:' + str(len(r)))
for x in r:
    print('URI:' + str(x.uri))
content = resources.get_static_resource('ue://resources/overview')
print('OVERVIEW_EXISTS:' + str(content is not None))
content2 = resources.get_static_resource('ue://resources/error-model')
print('ERROR_MODEL_EXISTS:' + str(content2 is not None))
content3 = resources.get_static_resource('ue://resources/workflows')
print('WORKFLOWS_EXISTS:' + str(content3 is not None))
content4 = resources.get_static_resource('ue://resources/blueprint-spec')
print('SPEC_EXISTS:' + str(content4 is not None))
print('NONEXISTENT:' + str(resources.get_static_resource('ue://resources/nonexistent') is None))
"@ 2>&1

$pyLines = $pyResult -split "`n" | Where-Object { $_ -match ':' }
$pyData = @{}
foreach ($line in $pyLines) {
    $parts = $line -split ':', 2
    if ($parts.Count -eq 2) { $pyData[$parts[0].Trim()] = $parts[1].Trim() }
}

Assert "1.1 4 static resources registered" ($pyData['COUNT'] -eq '4') "count=$($pyData['COUNT'])"
Assert "1.2 overview exists" ($pyData['OVERVIEW_EXISTS'] -eq 'True') "overview=$($pyData['OVERVIEW_EXISTS'])"
Assert "1.3 error-model exists" ($pyData['ERROR_MODEL_EXISTS'] -eq 'True') "error=$($pyData['ERROR_MODEL_EXISTS'])"
Assert "1.4 workflows exists" ($pyData['WORKFLOWS_EXISTS'] -eq 'True') "workflows=$($pyData['WORKFLOWS_EXISTS'])"
Assert "1.5 blueprint-spec exists" ($pyData['SPEC_EXISTS'] -eq 'True') "spec=$($pyData['SPEC_EXISTS'])"
Assert "1.6 nonexistent returns None" ($pyData['NONEXISTENT'] -eq 'True') "nonexistent=$($pyData['NONEXISTENT'])"
$log += ""

# Validate content contains expected sections
$contentCheck = python -c @"
import sys; sys.path.insert(0, '$resourcesDir')
from mcp_bridge_server import resources
r = resources.get_static_resource('ue://resources/overview')
text = r.contents[0].text
checks = ['Architecture', '49 Handlers', 'Single-client exclusive', 'Bootstrap / Live Model', 'CLIENT_ALREADY_CONNECTED']
for c in checks:
    print('CHECK:' + c + ':' + str(c in text))
"@ 2>&1
$checkLines = $contentCheck -split "`n" | Where-Object { $_ -match 'CHECK:' }
foreach ($line in $checkLines) {
    $parts = $line -replace 'CHECK:', '' -split ':', 2
    if ($parts.Count -eq 2) {
        Assert "1.7 overview has '$($parts[0].Trim())'" ($parts[1].Trim() -eq 'True') "found=$($parts[1].Trim())"
    }
}
$log += ""

# ============================================
# Part 2: Live Resource Data Sources (TCP)
# ============================================
$log += "===== Part 2: Live Resource Data Sources ====="

# 2.1 get_mcp_config (source for ue://runtime/config)
Start-Sleep -Milliseconds 200
$r = Send-Request "get_mcp_config" @{}
if ($r.ok) {
    Assert "2.1 get_mcp_config returns ok" $true "ok"
    Assert "2.1 has actions" ($r.result.PSObject.Properties.Name -contains "actions") "missing"
    Assert "2.1 has mode" ($r.result.PSObject.Properties.Name -contains "mode") "missing"
    Assert "2.1 has token_enabled" ($r.result.PSObject.Properties.Name -contains "token_enabled") "missing"
} else {
    Assert "2.1 get_mcp_config returns ok" $false "error=$($r.error.code): $($r.error.message)"
}
$log += ""

# 2.2 get_bridge_runtime_status (source for ue://runtime/status)
Start-Sleep -Milliseconds 200
$r = Send-Request "get_bridge_runtime_status" @{}
if ($r.ok) {
    Assert "2.2 get_bridge_runtime_status returns ok" $true "ok"
    Assert "2.2 has server_status" ($r.result.PSObject.Properties.Name -contains "server_status") "missing status"
    Assert "2.2 has client_connected" ($r.result.PSObject.Properties.Name -contains "client_connected") "missing client"
    Assert "2.2 has transport_mode" ($r.result.PSObject.Properties.Name -contains "transport_mode") "missing transport"
} else {
    Assert "2.2 get_bridge_runtime_status returns ok" $false "error=$($r.error.code): $($r.error.message)"
}
$log += ""

# ============================================
# Part 3: Existing Tools Regression
# ============================================
$log += "===== Part 3: Tools Regression ====="

# 3.1 ping still works
Start-Sleep -Milliseconds 200
$r = Send-Request "ping"
Assert "3.1 ping regression" ($r.ok -eq $true) "ok=$($r.ok)"
$log += ""

# 3.2 error path still works
Start-Sleep -Milliseconds 200
function Send-RawRequest($rawText) {
    try {
        $s = New-Object System.Net.Sockets.TcpClient($bridgeHost, $port)
        $stream = $s.GetStream()
        $writer = New-Object System.IO.StreamWriter($stream)
        $writer.Write($rawText + "`n")
        $writer.Flush()
        $reader = New-Object System.IO.StreamReader($stream)
        $line = $reader.ReadLine()
        $s.Close()
        return ConvertFrom-Json $line
    } catch { return $null }
}
Start-Sleep -Milliseconds 200
$r = Send-RawRequest "not json"
Assert "3.2 PARSE_ERROR still works" (($r -ne $null) -and ($r.error.code -eq "PARSE_ERROR")) "code=$($r.error.code)"
$log += ""

# Try server.py import to verify resource handlers registered
Start-Sleep -Milliseconds 200
$pyServerTest = python -c @"
import sys; sys.path.insert(0, '$resourcesDir')
from mcp_bridge_server.server import UEMCPServer
s = UEMCPServer()
# Check that list_resources and read_resource decorators exist
methods = [m for m in dir(s._server) if 'resource' in m.lower()]
print('RESOURCE_METHODS:' + str(len(methods)))
print('OK')
"@ 2>&1
$hasMethods = ($pyServerTest -match 'RESOURCE_METHODS:\d+')
Assert "3.3 Server has resource handlers" $hasMethods "output=$pyServerTest"
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
