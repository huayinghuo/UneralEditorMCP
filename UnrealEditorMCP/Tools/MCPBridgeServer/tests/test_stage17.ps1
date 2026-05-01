$bridgeHost = "127.0.0.1"
$port = 9876
$outFile = "E:\UEProject\UnrealEditorMCP\stage17_test_result.txt"

$s = New-Object System.Net.Sockets.TcpClient($bridgeHost, $port)
$stream = $s.GetStream()
$w = New-Object System.IO.StreamWriter($stream)
$rd = New-Object System.IO.StreamReader($stream)

function Send($action, $payload) {
    $script:seq++
    $req = @{ id = $script:seq.ToString(); action = $action; payload = $payload }
    $json = (ConvertTo-Json -Compress -Depth 5 $req) + "`n"
    $script:w.Write($json)
    $script:w.Flush()
    return ConvertFrom-Json ($script:rd.ReadLine())
}
$seq = 0
$log = @()
Start-Sleep -Milliseconds 300

$ts = Get-Date -Format "HHmmss"
$bpName = "TS17_$ts"

# 1. Create test BP
$r = Send "blueprint_create_actor_class" @{ name = $bpName; parent_class = "Actor" }
$log += "1. create_bp: ok=$($r.ok)"
if ($r.ok) { $bpPath = $r.result.asset_path; $log += "   path=$bpPath" } else { $log += "   err=$($r.error.code)" }

# Add variable Score for variable_node tests
Send "blueprint_add_variable" @{ asset_path = $bpPath; var_name = "Score"; var_type = "int" } | Out-Null

# 2. Apply spec with node_by_class
$spec = @{
    blueprint = @{ name = "${bpName}_nodespec"; parent_class = "Actor" }
    nodes = @(
        @{ id = "b"; type = "event"; event_name = "ReceiveBeginPlay" }
        @{ id = "br"; type = "node_by_class"; node_class = "K2Node_IfThenElse" }
        @{ id = "p1"; type = "call_function"; function_name = "PrintString"; params = @{ InString = "True" } }
    )
    edges = @(
        @{ from_node = "b"; from_pin = "then"; to_node = "br"; to_pin = "execute" }
    )
}
$r = Send "blueprint_apply_spec" @{ spec = $spec }
$log += "2. apply_spec(node_by_class): ok=$($r.ok)"
if ($r.ok) { $log += "   nodes=$($r.result.nodes_created) edges=$($r.result.edges_created)" } else { $log += "   err=$($r.error.code): $($r.error.message)" }

# 3. Apply spec with variable_set (expect VARIABLE_NOT_FOUND — new BP has no vars, dispatch verified)
$spec2 = @{
    blueprint = @{ name = "${bpName}_vset"; parent_class = "Actor" }
    nodes = @(
        @{ id = "s"; type = "variable_set"; var_name = "UnknownVar" }
    )
    edges = @()
}
$r = Send "blueprint_apply_spec" @{ spec = $spec2 }
$log += "3. apply_spec(variable_set): ok=$($r.ok)"
if ($r.ok) { $log += "   nodes=$($r.result.nodes_created)" } else { $log += "   err=$($r.error.code)" }

# 4. Apply spec with variable_get (expect VARIABLE_NOT_FOUND — dispatch verified)
$spec3 = @{
    blueprint = @{ name = "${bpName}_vget"; parent_class = "Actor" }
    nodes = @(
        @{ id = "g"; type = "variable_get"; var_name = "UnknownVar" }
    )
    edges = @()
}
$r = Send "blueprint_apply_spec" @{ spec = $spec3 }
$log += "4. apply_spec(variable_get): ok=$($r.ok)"
if ($r.ok) { $log += "   nodes=$($r.result.nodes_created)" } else { $log += "   err=$($r.error.code)" }

# --- Error tests ---
# 5. UNSUPPORTED_TYPE
$r = Send "blueprint_apply_spec" @{ spec = @{ blueprint = @{ name = "B1_$ts" }; nodes = @(@{ id = "x"; type = "bad_type" }); edges = @() } }
$log += "5. err-unsupported: ok=$($r.ok) code=$($r.error.code)"

# 6. node_by_class bad class
$r = Send "blueprint_apply_spec" @{ spec = @{ blueprint = @{ name = "B2_$ts" }; nodes = @(@{ id = "x"; type = "node_by_class"; node_class = "NotReal" }); edges = @() } }
$log += "6. err-bad_class: ok=$($r.ok) code=$($r.error.code)"

# 7. variable_get with missing var
$r = Send "blueprint_apply_spec" @{ spec = @{ blueprint = @{ name = "B3_$ts" }; nodes = @(@{ id = "x"; type = "variable_get"; var_name = "NoSuch" }); edges = @() } }
$log += "7. err-var_not_found: ok=$($r.ok) code=$($r.error.code)"

# 8. missing var_name
$r = Send "blueprint_apply_spec" @{ spec = @{ blueprint = @{ name = "B4_$ts" }; nodes = @(@{ id = "x"; type = "variable_set" }); edges = @() } }
$log += "8. err-missing_var: ok=$($r.ok) code=$($r.error.code)"

# 9. node_by_class missing node_class
$r = Send "blueprint_apply_spec" @{ spec = @{ blueprint = @{ name = "B5_$ts" }; nodes = @(@{ id = "x"; type = "node_by_class" }); edges = @() } }
$log += "9. err-missing_class: ok=$($r.ok) code=$($r.error.code)"

$s.Close()
$log | Out-File -FilePath $outFile -Encoding utf8
Write-Host "DONE"
