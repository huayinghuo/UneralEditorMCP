$bridgeHost = "127.0.0.1"
$port = 9876
$outFile = "E:\UEProject\UnrealEditorMCP\stage11b_test_result.txt"

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
Start-Sleep -Milliseconds 500

$ts = Get-Date -Format "HHmmss"
$bpName = "TestBPA_11B_$ts"

# ---- 1. blueprint_apply_spec ----
$spec = @{
    blueprint = @{ name = $bpName; parent_class = "Actor" }
    nodes = @(
        @{ id = "beginplay"; type = "event"; event_name = "ReceiveBeginPlay" }
        @{ id = "print"; type = "call_function"; function_name = "PrintString"; params = @{ InString = "Hello from 11B Spec!" } }
    )
    edges = @(
        @{ from_node = "beginplay"; from_pin = "then"; to_node = "print"; to_pin = "execute" }
    )
}

$r = Send-Request "blueprint_apply_spec" @{ spec = $spec }
$log += "1. apply_spec: ok=$($r.ok)"
if ($r.ok) {
    $bpPath = $r.result.asset_path
    $log += "   asset_path=$bpPath nodes_created=$($r.result.nodes_created) edges_created=$($r.result.edges_created) saved=$($r.result.saved)"
} else {
    $log += "   error=$($r.error.code): $($r.error.message)"
    $bpPath = ""
}

# ---- 2. blueprint_compile_save (verify) ----
if ($bpPath) {
    $r = Send-Request "blueprint_compile_save" @{ asset_path = $bpPath; save = $true }
    $log += "2. compile: ok=$($r.ok) compiled=$($r.result.compiled) saved=$($r.result.saved)"
}

# ---- 3. blueprint_export_spec ----
if ($bpPath) {
    $r = Send-Request "blueprint_export_spec" @{ asset_path = $bpPath }
    $log += "3. export_spec: ok=$($r.ok)"
    if ($r.ok) {
        $log += "   node_count=$($r.result.node_count) edge_count=$($r.result.edge_count)"
        $log += "   bp_name=$($r.result.blueprint.name) parent_class=$($r.result.blueprint.parent_class)"
    } else {
        $log += "   error=$($r.error.code): $($r.error.message)"
    }
}

# ---- 4. apply_spec with existing BP (expect ASSET_CONFLICT) ----
$spec2 = @{
    blueprint = @{ name = $bpName; parent_class = "Actor" }
    nodes = @()
    edges = @()
}
$r = Send-Request "blueprint_apply_spec" @{ spec = $spec2 }
$log += "4. apply_duplicate: ok=$($r.ok) error_code=$($r.error.code)"

# ---- 5. apply_spec bad function ----
$ts2 = Get-Date -Format "HHmmss"
$spec3 = @{
    blueprint = @{ name = "BadFunc_$ts2"; parent_class = "Actor" }
    nodes = @(@{ id = "n1"; type = "call_function"; function_name = "NonExistentFuncZZZ" })
    edges = @()
}
$r = Send-Request "blueprint_apply_spec" @{ spec = $spec3 }
$log += "5. apply_bad_func: ok=$($r.ok) error_code=$($r.error.code)"

# ---- 6. apply_spec bad parent ----
$ts3 = Get-Date -Format "HHmmss"
$spec4 = @{
    blueprint = @{ name = "BadParent_$ts3"; parent_class = "NotAnActorClass" }
    nodes = @()
    edges = @()
}
$r = Send-Request "blueprint_apply_spec" @{ spec = $spec4 }
$log += "6. apply_bad_parent: ok=$($r.ok) error_code=$($r.error.code)"

# ---- 7. apply_spec missing spec field ----
$r = Send-Request "blueprint_apply_spec" @{ }
$log += "7. apply_missing_spec: ok=$($r.ok) error_code=$($r.error.code)"

$log | Out-File -FilePath $outFile -Encoding utf8
Write-Host "DONE"
