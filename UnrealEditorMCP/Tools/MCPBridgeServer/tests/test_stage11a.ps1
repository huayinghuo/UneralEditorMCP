$bridgeHost = "127.0.0.1"
$port = 9876
$outFile = "E:\UEProject\UnrealEditorMCP\stage11a_test_result.txt"

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

$ts = Get-Date -Format "HHmmss"
$bpName = "TestBPA_Stage11A_$ts"

Start-Sleep -Milliseconds 500

# 1. ping
$r = Send-Request "ping"
$log += "1. ping: ok=$($r.ok)"

# 2. blueprint_create_actor_class
$r = Send-Request "blueprint_create_actor_class" @{ name = $bpName; parent_class = "Actor" }
$log += "2. create_actor_bp: ok=$($r.ok)"
if ($r.ok) {
    $bpPath = $r.result.asset_path
    $log += "   asset_path=$bpPath"
} else {
    $log += "   error=$($r.error.code): $($r.error.message)"
    $bpPath = ""
}

# 3. blueprint_get_event_graph_info
if ($bpPath) {
    $r = Send-Request "blueprint_get_event_graph_info" @{ asset_path = $bpPath }
    $log += "3. event_graph_info: ok=$($r.ok)"
    if ($r.ok) {
        $log += "   exists=$($r.result.exists) has_event_graph=$($r.result.has_event_graph) event_node_count=$($r.result.event_node_count)"
    }
}

# 4. blueprint_add_event_node
$eventGuid = ""
if ($bpPath) {
    $r = Send-Request "blueprint_add_event_node" @{ asset_path = $bpPath; event_name = "ReceiveBeginPlay" }
    $log += "4. add_event_node: ok=$($r.ok)"
    if ($r.ok) {
        $eventGuid = $r.result.node_guid
        $log += "   node_guid=$eventGuid created=$($r.result.created)"
    } else {
        $log += "   error=$($r.error.code): $($r.error.message)"
    }
}

# 5. blueprint_add_call_function_node
$callGuid = ""
if ($bpPath) {
    $r = Send-Request "blueprint_add_call_function_node" @{ asset_path = $bpPath; function_name = "PrintString" }
    $log += "5. add_call_function: ok=$($r.ok)"
    if ($r.ok) {
        $callGuid = $r.result.node_guid
        $log += "   node_guid=$callGuid"
    } else {
        $log += "   error=$($r.error.code): $($r.error.message)"
    }
}

# 6. blueprint_connect_pins
if ($eventGuid -and $callGuid) {
    $r = Send-Request "blueprint_connect_pins" @{
        asset_path = $bpPath
        source_node_guid = $eventGuid
        source_pin_name = "then"
        target_node_guid = $callGuid
        target_pin_name = "execute"
    }
    $log += "6. connect_pins: ok=$($r.ok)"
    if ($r.ok) {
        $log += "   connected=$($r.result.connected)"
    } else {
        $log += "   error=$($r.error.code): $($r.error.message)"
    }
} else {
    $log += "6. connect_pins: SKIPPED (event_guid=$eventGuid, call_guid=$callGuid)"
}

# 7. blueprint_compile_save
if ($bpPath) {
    $r = Send-Request "blueprint_compile_save" @{ asset_path = $bpPath; save = $true }
    $log += "7. compile_save: ok=$($r.ok)"
    if ($r.ok) {
        $log += "   compiled=$($r.result.compiled) saved=$($r.result.saved)"
    } else {
        $log += "   error=$($r.error.code): $($r.error.message)"
    }
}

# --- Error case tests ---
$log += ""

# 8. Bad parent class
$r = Send-Request "blueprint_create_actor_class" @{ name = "BadTest"; parent_class = "NotAnActorClass" }
$log += "8. create_bad_parent: ok=$($r.ok) error_code=$($r.error.code)"

# 9. Bad function name
if ($bpPath) {
    $r = Send-Request "blueprint_add_call_function_node" @{ asset_path = $bpPath; function_name = "NonExistentFuncXYZ" }
    $log += "9. bad_func_name: ok=$($r.ok) error_code=$($r.error.code)"
}

# 10. Bad node GUIDs for connect
if ($bpPath) {
    $r = Send-Request "blueprint_connect_pins" @{
        asset_path = $bpPath
        source_node_guid = "00000000-0000-0000-0000-000000000000"
        source_pin_name = "then"
        target_node_guid = "11111111-1111-1111-1111-111111111111"
        target_pin_name = "execute"
    }
    $log += "10. bad_guids: ok=$($r.ok) error_code=$($r.error.code)"
}

$log | Out-File -FilePath $outFile -Encoding utf8
Write-Host "DONE"
