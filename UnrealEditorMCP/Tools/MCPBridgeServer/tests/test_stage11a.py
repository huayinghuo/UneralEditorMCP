import sys, json, time, os
sys.path.insert(0, r"E:\UEProject\UnrealEditorMCP\UnrealEditorMCP\Tools\MCPBridgeServer\src")
from mcp_bridge_server.bridge_client import UEBridgeClient

OUT = r"E:\UEProject\UnrealEditorMCP\stage11a_test_result.txt"
LOG = []

def log(msg):
    LOG.append(msg)

def send(action, payload=None):
    try:
        r = c.send(action, payload or {})
        return r
    except Exception as e:
        return {"ok": False, "error": {"code": "EXCEPTION", "message": str(e)}}

def run():
    time.sleep(0.5)
    global c
    try:
        c = UEBridgeClient()
    except Exception as e:
        LOG.append(f"FATAL: Cannot connect: {e}")
        return

    # ---- 1. ping ----
    r = send("ping")
    log(f"1. ping: ok={r.get('ok')}")

    # ---- 2. blueprint_create_actor_class ----
    r = send("blueprint_create_actor_class", {"name": "TestBPA_Stage11A", "parent_class": "Actor"})
    log(f"2. create_actor_bp: ok={r.get('ok')} result={json.dumps(r.get('result',{}), ensure_ascii=False)}")
    bp_path = r.get("result", {}).get("asset_path", "")

    # ---- 3. blueprint_get_event_graph_info ----
    r = send("blueprint_get_event_graph_info", {"asset_path": bp_path})
    log(f"3. event_graph_info: ok={r.get('ok')} result={json.dumps(r.get('result',{}), ensure_ascii=False)}")

    # ---- 4. blueprint_add_event_node ----
    r = send("blueprint_add_event_node", {"asset_path": bp_path, "event_name": "ReceiveBeginPlay"})
    log(f"4. add_event_node: ok={r.get('ok')} result={json.dumps(r.get('result',{}), ensure_ascii=False)}")
    event_guid = r.get("result", {}).get("node_guid", "")

    # ---- 5. blueprint_add_call_function_node ----
    r = send("blueprint_add_call_function_node", {"asset_path": bp_path, "function_name": "PrintString"})
    log(f"5. add_call_function: ok={r.get('ok')} result={json.dumps(r.get('result',{}), ensure_ascii=False)}")
    call_guid = r.get("result", {}).get("node_guid", "")

    # ---- 6. blueprint_connect_pins ----
    if event_guid and call_guid:
        r = send("blueprint_connect_pins", {
            "asset_path": bp_path,
            "source_node_guid": event_guid,
            "source_pin_name": "then",
            "target_node_guid": call_guid,
            "target_pin_name": "execute"
        })
        log(f"6. connect_pins: ok={r.get('ok')} result={json.dumps(r.get('result',{}), ensure_ascii=False)}")
    else:
        log(f"6. connect_pins: SKIPPED (missing guids: event={event_guid}, call={call_guid})")

    # ---- 7. blueprint_compile_save ----
    r = send("blueprint_compile_save", {"asset_path": bp_path, "save": True})
    log(f"7. compile_save: ok={r.get('ok')} result={json.dumps(r.get('result',{}), ensure_ascii=False)}")

    # ---- error-case tests ----
    r = send("blueprint_create_actor_class", {"name": "TestBPA_Stage11A", "parent_class": "NotAnActor"})
    log(f"8. create_bad_parent: ok={r.get('ok')} error={json.dumps(r.get('error',{}), ensure_ascii=False)}")

    r = send("blueprint_add_call_function_node", {"asset_path": bp_path, "function_name": "NonExistentFuncXYZ"})
    log(f"9. bad_func_name: ok={r.get('ok')} error={json.dumps(r.get('error',{}), ensure_ascii=False)}")

    r = send("blueprint_connect_pins", {
        "asset_path": bp_path,
        "source_node_guid": "00000000-0000-0000-0000-000000000000",
        "source_pin_name": "then",
        "target_node_guid": "11111111-1111-1111-1111-111111111111",
        "target_pin_name": "execute"
    })
    log(f"10. bad_guids: ok={r.get('ok')} error={json.dumps(r.get('error',{}), ensure_ascii=False)}")

    c.disconnect()
    log("\n=== STAGE 11A ACCEPTANCE TEST COMPLETE ===")

if __name__ == "__main__":
    try:
        c = None
        run()
    except Exception as e:
        LOG.append(f"FATAL: {e}")
    finally:
        with open(OUT, "w", encoding="utf-8") as f:
            f.write("\n".join(LOG))
