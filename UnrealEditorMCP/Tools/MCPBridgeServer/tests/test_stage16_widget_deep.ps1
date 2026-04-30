# Stage 16 Widget Deepening Acceptance Test
# Tests: create with root, tree build, slot editing, property editing,
#        reparent, reorder, rename, schema query, error paths
# All tests use PowerShell direct TCP

$bridgeHost = "127.0.0.1"
$port = 9876
$testDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$outFile = Join-Path $testDir "stage16_test_result.txt"
$bpName = "WBP_MCP_WidgetDeepDive"
$bpPath = "/Game/UI/$bpName"

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

$log = @(); $failCount = 0; $passCount = 0
function Assert($label, $condition, $detail) {
    if ($condition) { $script:passCount++; $script:log += "PASS: $label" }
    else { $script:failCount++; $script:log += "FAIL: $label -- $detail" }
}

# Wait for bridge
for ($i=0; $i -lt 30; $i++) { try { $c = New-Object System.Net.Sockets.TcpClient('127.0.0.1', 9876); $c.Close(); break } catch { Start-Sleep -Seconds 3 } }
Start-Sleep -Seconds 3

# ===== Part 1: Create with root =====
$log += "===== Part 1: Create with Root ====="
Start-Sleep -Milliseconds 500
$r = Send-Request "create_widget_blueprint" @{ name = $bpName; path = "/Game/UI"; root_widget_class = "CanvasPanel" }
$log += "1. create: ok=$($r.ok)"
if ($r.ok) {
    Assert "1.0 created" $true "path=$($r.result.path)"
    $log += "   root=$($r.result.root_widget) class=$($r.result.root_widget_class)"
    Assert "1.1 has root" ($r.result.PSObject.Properties.Name -contains "root_widget") "missing"
} else {
    Assert "1.0 created" $false "error=$($r.error.code): $($r.error.message)"
    $log += "   (asset may already exist, continuing)"
}
$log += ""

# ===== Part 2: Build tree =====
$log += "===== Part 2: Build Tree ====="
$vboxOk = $true
Start-Sleep -Seconds 2
$r = Send-Request "widget_add_child" @{ asset_path = "$bpPath.$bpName"; widget_class = "VerticalBox"; widget_name = "RootVBox" }
Assert "2.1 add RootVBox" $r.ok "error=$($r.error.code)"
$log += ""

Start-Sleep -Seconds 2
$r = Send-Request "widget_add_child" @{ asset_path = "$bpPath.$bpName"; parent_widget = "RootVBox"; widget_class = "TextBlock"; widget_name = "TitleText" }
Assert "2.2 add TitleText" $r.ok "error=$($r.error.code)"

Start-Sleep -Seconds 2
$r = Send-Request "widget_add_child" @{ asset_path = "$bpPath.$bpName"; parent_widget = "RootVBox"; widget_class = "Image"; widget_name = "HeroImage" }
Assert "2.3 add HeroImage" $r.ok "error=$($r.error.code)"

Start-Sleep -Seconds 2
$r = Send-Request "widget_add_child" @{ asset_path = "$bpPath.$bpName"; parent_widget = "RootVBox"; widget_class = "Button"; widget_name = "ConfirmButton" }
Assert "2.4 add ConfirmButton" $r.ok "error=$($r.error.code)"

Start-Sleep -Seconds 2
$r = Send-Request "widget_add_child" @{ asset_path = "$bpPath.$bpName"; parent_widget = "ConfirmButton"; widget_class = "TextBlock"; widget_name = "ConfirmLabel" }
Assert "2.5 add ConfirmLabel" $r.ok "error=$($r.error.code)"
$log += ""

# ===== Part 3: Query tree =====
$log += "===== Part 3: Query Tree ====="
Start-Sleep -Seconds 3
$r = Send-Request "get_widget_info" @{ asset_path = "$bpPath.$bpName" }
Assert "3.1 get_widget_info" $r.ok "error=$($r.error.code)"
if ($r.ok) {
    $log += "   root=$($r.result.root_widget) class=$($r.result.root_widget_class)"
    Assert "3.2 has root_widget" ($r.result.PSObject.Properties.Name -contains "root_widget") "missing"
}

Start-Sleep -Seconds 3
$r = Send-Request "widget_find" @{ asset_path = "$bpPath.$bpName"; query = "Title" }
Assert "3.3 find Title*" ($r.ok -and $r.result.count -gt 0) "count=$($r.result.count)"

Start-Sleep -Seconds 3
$r = Send-Request "widget_get_property_schema" @{ asset_path = "$bpPath.$bpName"; widget_name = "TitleText" }
Assert "3.4 property schema" $r.ok "error=$($r.error.code)"
if ($r.ok) { $log += "   class=$($r.result.widget_class) count=$($r.result.count)" }

Start-Sleep -Seconds 3
$r = Send-Request "widget_get_slot_schema" @{ asset_path = "$bpPath.$bpName"; widget_name = "TitleText" }
Assert "3.5 slot schema" $r.ok "error=$($r.error.code)"
if ($r.ok) { $log += "   slot=$($r.result.slot_class) count=$($r.result.count)" }
$log += ""

# ===== Part 4: Property & Slot editing =====
$log += "===== Part 4: Property & Slot Editing ====="
Start-Sleep -Seconds 3
$r = Send-Request "widget_set_property" @{ asset_path = "$bpPath.$bpName"; widget_name = "TitleText"; property_name = "Text"; value = "Hello MCP" }
Assert "4.1 set Text" $r.ok "error=$($r.error.code)"

Start-Sleep -Seconds 3
$r = Send-Request "widget_set_slot_property" @{ asset_path = "$bpPath.$bpName"; widget_name = "TitleText"; property_name = "Padding"; value = "(Left=10,Top=5,Right=10,Bottom=5)" }
Assert "4.2 set slot padding" $r.ok "error=$($r.error.code)"

Start-Sleep -Seconds 3
$r = Send-Request "widget_set_slot_property" @{ asset_path = "$bpPath.$bpName"; widget_name = "TitleText"; property_name = "HorizontalAlignment"; value = "HAlign_Center" }
Assert "4.3 set slot h-align" $r.ok "error=$($r.error.code)"
$log += ""

# ===== Part 5: Structure editing =====
$log += "===== Part 5: Structure Editing ====="
Start-Sleep -Seconds 3
$r = Send-Request "widget_rename" @{ asset_path = "$bpPath.$bpName"; widget_name = "ConfirmLabel"; new_name = "ConfirmText" }
Assert "5.1 rename ConfirmLabel->ConfirmText" $r.ok "error=$($r.error.code)"

Start-Sleep -Seconds 3
$r = Send-Request "widget_duplicate" @{ asset_path = "$bpPath.$bpName"; widget_name = "TitleText" }
Assert "5.2 duplicate TitleText" $r.ok "error=$($r.error.code)"

Start-Sleep -Seconds 3
$r = Send-Request "widget_reorder_child" @{ asset_path = "$bpPath.$bpName"; widget_name = "HeroImage"; index = 0 }
Assert "5.3 reorder HeroImage to index 0" $r.ok "error=$($r.error.code)"

Start-Sleep -Seconds 3
$r = Send-Request "widget_set_root" @{ asset_path = "$bpPath.$bpName"; widget_class = "SizeBox" }
Assert "5.4 replace root with SizeBox" $r.ok "error=$($r.error.code)"
if ($r.ok) { $log += "   replaced=$($r.result.replaced)" }
$log += ""

# ===== Part 6: Wrap =====
$log += "===== Part 6: Wrap ====="
Start-Sleep -Seconds 3
$r = Send-Request "widget_wrap_with_panel" @{ asset_path = "$bpPath.$bpName"; widget_name = "ConfirmButton"; panel_class = "Border"; wrapper_name = "ButtonBorder" }
Assert "6.1 wrap ConfirmButton in Border" $r.ok "error=$($r.error.code)"
$log += ""

# ===== Part 7: Error paths =====
$log += "===== Part 7: Error Paths ====="
Start-Sleep -Seconds 3
$r = Send-Request "widget_set_root" @{ asset_path = "$bpPath.$bpName"; widget_class = "NotAWidgetClass" }
Assert "7.1 CLASS_NOT_FOUND for bad root" ((-not $r.ok) -and ($r.error.code -eq "CLASS_NOT_FOUND")) "code=$($r.error.code)"

Start-Sleep -Seconds 3
$r = Send-Request "widget_reparent" @{ asset_path = "$bpPath.$bpName"; widget_name = "RootVBox"; parent_widget = "ConfirmButton" }
Assert "7.2 REPARENT_CYCLE" ((-not $r.ok) -and ($r.error.code -match "CYCLE|PANEL")) "code=$($r.error.code)"

Start-Sleep -Seconds 3
$r = Send-Request "widget_rename" @{ asset_path = "$bpPath.$bpName"; widget_name = "HeroImage"; new_name = "TitleText" }
Assert "7.3 WIDGET_NAME_CONFLICT" ((-not $r.ok) -and ($r.error.code -eq "WIDGET_NAME_CONFLICT")) "code=$($r.error.code)"

Start-Sleep -Seconds 3
$r = Send-Request "widget_set_property" @{ asset_path = "$bpPath.$bpName"; widget_name = "TitleText"; property_name = "DoesNotExist"; value = "x" }
Assert "7.4 PROPERTY_NOT_FOUND" ((-not $r.ok) -and ($r.error.code -eq "PROPERTY_NOT_FOUND")) "code=$($r.error.code)"

Start-Sleep -Seconds 3
$r = Send-Request "widget_set_slot_property" @{ asset_path = "$bpPath.$bpName"; widget_name = "TitleText"; property_name = "DoesNotExist"; value = "x" }
Assert "7.5 SLOT_PROPERTY_NOT_SUPPORTED" ((-not $r.ok)) "code=$($r.error.code)"
$log += ""

# ===== Part 8: Final state ====="
$log += "===== Part 8: Final State ====="
Start-Sleep -Seconds 3
$r = Send-Request "get_widget_info" @{ asset_path = "$bpPath.$bpName" }
Assert "8.1 final get_widget_info" $r.ok "error=$($r.error.code)"

Start-Sleep -Seconds 3
$r = Send-Request "blueprint_compile_save" @{ asset_path = "$bpPath.$bpName"; save = $true }
Assert "8.2 compile save" $r.ok "error=$($r.error.code)"
$log += ""

# ===== Summary =====
$log += "===== SUMMARY ====="
$log += "Passed: $passCount"
$log += "Failed: $failCount"
$total = $passCount + $failCount
$log += "Total:  $total"
if ($failCount -eq 0) { $log += "RESULT: ALL TESTS PASSED" }
else { $log += "RESULT: $failCount TEST(S) FAILED" }

$log | Out-File -FilePath $outFile -Encoding utf8
Write-Host "DONE - $outFile"
Write-Host "Passed: $passCount, Failed: $failCount"
