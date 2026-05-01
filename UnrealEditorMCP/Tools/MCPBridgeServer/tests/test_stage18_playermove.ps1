$bridgeHost = "127.0.0.1"
$port = 9876
$outFile = "E:\UEProject\UnrealEditorMCP\stage18_test_result.txt"

$s = New-Object System.Net.Sockets.TcpClient($bridgeHost, $port)
$stream = $s.GetStream()
$w = New-Object System.IO.StreamWriter($stream)
$rd = New-Object System.IO.StreamReader($stream)
$seq = 0
$log = @()

function Send($action, $payload) {
    $script:seq++
    $req = @{ id = $seq.ToString(); action = $action; payload = $payload }
    $json = (ConvertTo-Json -Compress -Depth 5 $req) + "`n"
    $script:w.Write($json); $script:w.Flush()
    return ConvertFrom-Json ($script:rd.ReadLine())
}

$ts = Get-Date -Format "HHmmss"
$bpName = "PlayerMove_$ts"
$iaName = "IA_Move_$ts"
$imcName = "IMC_Move_$ts"

# 1. Create Character BP + variable
$r = Send "blueprint_create_actor_class" @{name=$bpName;parent_class="Character"}
$log += "1. create_bp: ok=$($r.ok)"; $bp = $r.result.asset_path
Send "blueprint_add_variable" @{asset_path=$bp;var_name="Speed";var_type="float";default_value="600.0"} | Out-Null

# 2. Create InputAction + IMC + mapping
Send "create_input_action" @{name=$iaName;value_type="vector2d"} | Out-Null
Send "create_input_mapping_context" @{name=$imcName} | Out-Null
$iaPath = "/Game/MCPTest/$iaName.$iaName"
$imcPath = "/Game/MCPTest/$imcName.$imcName"
$r = Send "add_input_mapping" @{context_path=$imcPath;action_path=$iaPath;key="W"}
$log += "2. ei_setup: add_mapping=$($r.ok)"

# 3. Add Enhanced Input node to BP
$r = Send "blueprint_add_enhanced_input_node" @{asset_path=$bp;action_path=$iaPath}
$log += "3. ei_node: ok=$($r.ok)"; $eiGuid = if($r.ok){$r.result.node_guid}else{""}

# 4. Add IMC node to BP
$r = Send "blueprint_add_imc_node" @{asset_path=$bp;context_path=$imcPath}
$log += "4. imc_node: ok=$($r.ok)"

# 5. Add CallFunction (AddMovementInput) + connect
$r = Send "blueprint_add_event_node" @{asset_path=$bp;event_name="ReceiveBeginPlay"}
$log += "5. beginplay: ok=$($r.ok)"; $bpGuid = if($r.ok){$r.result.node_guid}else{""}
$r = Send "blueprint_add_call_function_node" @{asset_path=$bp;function_name="AddMovementInput"}
$log += "6. move_input: ok=$($r.ok)"; $moveGuid = if($r.ok){$r.result.node_guid}else{""}
if ($eiGuid -and $moveGuid) {
    $r = Send "blueprint_connect_pins" @{asset_path=$bp;source_node_guid=$eiGuid;source_pin_name="then";target_node_guid=$moveGuid;target_pin_name="execute"}
    $log += "7. connect: ok=$($r.ok)"
} else { $log += "7. connect: SKIPPED" }

# 6. Compile
$r = Send "blueprint_compile_save" @{asset_path=$bp;save=$true}
$log += "8. compile: ok=$($r.ok) compiled=$($r.result.compiled)"

# 7. set_level_default_pawn
$r = Send "set_level_default_pawn" @{pawn_class=$bpName}
$log += "9. set_pawn: ok=$($r.ok)"

# 8. PIE lifecycle
$r = Send "pie_start" @{}
$log += "10. pie_start: ok=$($r.ok)"
Start-Sleep -Seconds 3
$r = Send "pie_is_running" @{}
$log += "11. pie_running: ok=$($r.ok) running=$($r.result.running)"
$r = Send "get_actor_state" @{actor_name=$bpName}
$log += "12. actor_state: ok=$($r.ok)"
if ($r.ok) { $log += "   pos=($($r.result.position.x),$($r.result.position.y),$($r.result.position.z)) vel=($($r.result.velocity.x),$($r.result.velocity.y),$($r.result.velocity.z))" }
$r = Send "pie_stop" @{}
$log += "13. pie_stop: ok=$($r.ok)"

# 9. Error paths
$r = Send "pie_stop" @{}
$log += "14. err-stop_no_pie: ok=$($r.ok) code=$($r.error.code)"
$r = Send "get_actor_state" @{actor_name="Nothing"}
$log += "15. err-bad_actor: ok=$($r.ok) code=$($r.error.code)"
$r = Send "add_input_mapping" @{context_path=$imcPath;action_path="/Game/Input/Nope.Nope";key="X"}
$log += "16. err-bad_ia: ok=$($r.ok) code=$($r.error.code)"

$s.Close()
$log | Out-File -FilePath $outFile -Encoding utf8
Write-Host "DONE"
