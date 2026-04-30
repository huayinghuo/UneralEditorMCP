"""
Stage 14 MCP Resources Protocol Test
Tests resources/list and resources/read via MCP JSON-RPC over stdio subprocess.
Does NOT use python -c inline; runs as a standalone script.
"""
import subprocess
import json
import sys
import os
import time

# Ensure the src directory is on path
SRC_DIR = os.path.join(os.path.dirname(__file__), "..", "src")
sys.path.insert(0, SRC_DIR)

def find_server_script():
    """Locate the server module entry point."""
    server_py = os.path.join(SRC_DIR, "mcp_bridge_server", "server.py")
    if os.path.exists(server_py):
        return server_py
    raise FileNotFoundError(f"Cannot find server.py at {server_py}")

def send_jsonrpc(proc, method, params=None, request_id=1):
    """Send a JSON-RPC 2.0 request and return the response."""
    request = {
        "jsonrpc": "2.0",
        "id": request_id,
        "method": method,
        "params": params or {},
    }
    line = json.dumps(request) + "\n"
    proc.stdin.write(line)
    proc.stdin.flush()
    
    # Read response (may include notifications before the actual response)
    for _ in range(100):  # safety limit
        resp_line = proc.stdout.readline()
        if not resp_line:
            raise RuntimeError("Server closed stdout unexpectedly")
        try:
            resp = json.loads(resp_line)
            if resp.get("id") == request_id:
                return resp
            # Skip notifications (no id field in JSON-RPC)
        except json.JSONDecodeError:
            print(f"  [skip] Non-JSON line: {resp_line[:80]}...")
            continue
    raise RuntimeError(f"No matching response for id={request_id} after 100 lines")

def test_resources_list(proc):
    """Test resources/list returns >= 6 resources."""
    resp = send_jsonrpc(proc, "resources/list")
    if "error" in resp:
        return False, f"resources/list error: {resp['error']}"
    resources = resp.get("result", {}).get("resources", [])
    count = len(resources)
    if count < 6:
        return False, f"resources/list returned only {count} resources (expected >= 6)"
    
    uris = [r["uri"] for r in resources]
    required_static = [
        "ue://resources/overview",
        "ue://resources/error-model",
        "ue://resources/workflows",
        "ue://resources/blueprint-spec",
    ]
    required_live = [
        "ue://runtime/config",
        "ue://runtime/status",
    ]
    missing = [u for u in required_static + required_live if u not in uris]
    if missing:
        return False, f"resources/list missing URIs: {missing}"
    
    return True, f"{count} resources, all required URIs present"

def test_read_static_resource(proc, uri, expected_keywords):
    """Test resources/read for a static resource returns content with expected keywords."""
    resp = send_jsonrpc(proc, "resources/read", {"uri": uri}, request_id=hash(uri) % 10000 + 10)
    if "error" in resp:
        return False, f"resources/read {uri} error: {resp['error']}"
    
    contents = resp.get("result", {}).get("contents", [])
    if not contents:
        return False, f"resources/read {uri} returned empty contents"
    
    text = contents[0].get("text", "")
    missing_kw = [kw for kw in expected_keywords if kw not in text]
    if missing_kw:
        return False, f"resources/read {uri} missing keywords: {missing_kw}"
    
    return True, f"{uri}: all {len(expected_keywords)} keywords found"

def test_read_live_resource(proc, uri, expected_fields):
    """Test resources/read for a live resource returns JSON with expected fields."""
    resp = send_jsonrpc(proc, "resources/read", {"uri": uri}, request_id=hash(uri) % 10000 + 20)
    if "error" in resp:
        return False, f"resources/read {uri} error: {resp['error']}"
    
    contents = resp.get("result", {}).get("contents", [])
    if not contents:
        return False, f"resources/read {uri} returned empty contents"
    
    text = contents[0].get("text", "")
    try:
        data = json.loads(text)
    except json.JSONDecodeError as e:
        return False, f"resources/read {uri} returned invalid JSON: {e}"
    
    missing = [f for f in expected_fields if f not in data]
    if missing:
        return False, f"resources/read {uri} missing fields: {missing}"
    
    return True, f"{uri}: all {len(expected_fields)} fields present"

def run():
    server_script = find_server_script()
    print(f"Server script: {server_script}")
    
    # Launch MCP server as subprocess via python -m
    env = os.environ.copy()
    env["PYTHONPATH"] = SRC_DIR
    proc = subprocess.Popen(
        [sys.executable, "-u", "-m", "mcp_bridge_server.server"],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        encoding="utf-8",
        cwd=SRC_DIR,
        env=env,
    )
    
    results = []
    try:
        # JSON-RPC Initialize
        print("\n--- Initialize ---")
        init_resp = send_jsonrpc(proc, "initialize", {
            "protocolVersion": "2024-11-05",
            "capabilities": {},
            "clientInfo": {"name": "test", "version": "0.1"},
        }, request_id=0)
        if "error" in init_resp:
            results.append(("FAIL", "Initialize", f"Error: {init_resp['error']}"))
            return 1
        print("  OK: initialized")
        results.append(("PASS", "Initialize", "OK"))
        
        # Send "notifications/initialized" notification (required by MCP)
        notif = json.dumps({"jsonrpc": "2.0", "method": "notifications/initialized"}) + "\n"
        proc.stdin.write(notif)
        proc.stdin.flush()
        time.sleep(0.2)
        
        # Test 1: resources/list
        print("\n--- Test resources/list ---")
        ok, msg = test_resources_list(proc)
        print(f"  {'PASS' if ok else 'FAIL'}: {msg}")
        results.append(("PASS" if ok else "FAIL", "resources/list", msg))
        
        # Test 2-5: resources/read for each static resource
        static_tests = [
            ("ue://resources/overview", ["Architecture", "49 Handlers", "Single-client exclusive", "Bootstrap / Live Model"]),
            ("ue://resources/error-model", ["UEBridgeError", "CONNECT_TIMEOUT", "CLIENT_ALREADY_CONNECTED", "Client Guidance"]),
            ("ue://resources/workflows", ["ue_blueprint_create_actor_class", "ue_widget_add_child", "ue_begin_transaction", "ue_material_set_scalar_param"]),
            ("ue://resources/blueprint-spec", ["apply_spec", "export_spec", "Round-Trip", "Common Pitfalls"]),
        ]
        print("\n--- Test resources/read (static) ---")
        for uri, keywords in static_tests:
            ok, msg = test_read_static_resource(proc, uri, keywords)
            print(f"  {'PASS' if ok else 'FAIL'}: {msg}")
            results.append(("PASS" if ok else "FAIL", f"read:{uri.split('/')[-1]}", msg))
        
        # Test 6-7: resources/read for live resources
        live_tests = [
            ("ue://runtime/config", ["actions", "mode", "token_enabled"]),
            ("ue://runtime/status", ["server_status", "client_connected"]),
        ]
        print("\n--- Test resources/read (live) ---")
        for uri, fields in live_tests:
            ok, msg = test_read_live_resource(proc, uri, fields)
            print(f"  {'PASS' if ok else 'FAIL'}: {msg}")
            results.append(("PASS" if ok else "FAIL", f"read:{uri.split('/')[-1]}", msg))
    
    except Exception as e:
        print(f"\n  EXCEPTION: {e}")
        import traceback; traceback.print_exc()
        results.append(("FAIL", "Exception", str(e)))
    finally:
        proc.stdin.close()
        proc.terminate()
        try:
            proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            proc.kill()
    
    # Summary
    passed = sum(1 for r in results if r[0] == "PASS")
    failed = sum(1 for r in results if r[0] == "FAIL")
    print(f"\n===== SUMMARY =====")
    for status, name, detail in results:
        print(f"  {status}: {name} -- {detail}")
    print(f"Passed: {passed}, Failed: {failed}, Total: {len(results)}")
    
    return 0 if failed == 0 else 1


if __name__ == "__main__":
    sys.exit(run())
