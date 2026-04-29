import sys, json
sys.path.insert(0, r"E:\UEProject\UnrealEditorMCP\UnrealEditorMCP\Tools\MCPBridgeServer\src")
from mcp_bridge_server.bridge_client import UEBridgeClient

c = UEBridgeClient()

# Test 1: list_assets
print("=== list_assets (/Game, limit 3) ===")
r = c.send("list_assets", {"path": "/Game", "recursive": True})
result = r.get("result", {})
print(f"count: {result.get('count', '?')}")
for a in result.get("assets", [])[:3]:
    print(f"  {a['name']} ({a['class']})")

# Test 2: get_asset_info (if any assets)
if result.get("assets"):
    first_path = result["assets"][0]["path"]
    print()
    print(f"=== get_asset_info ({first_path}) ===")
    r2 = c.send("get_asset_info", {"asset_path": first_path})
    print(json.dumps(r2, indent=2, ensure_ascii=False))

# Test 3: Transaction - spawn, undo, redo
print()
print("=== Transaction test ===")
r3 = c.send("begin_transaction", {"description": "Spawn test actor for undo"})
print(f"begin: {r3.get('ok')}")
r4 = c.send("spawn_actor", {"class_name": "PointLight"})
actor_name = r4.get("result", {}).get("name", "?")
print(f"spawned: {actor_name}")
r5 = c.send("end_transaction")
print(f"end: {r5.get('ok')}")
r6 = c.send("undo")
print(f"undo: {r6.get('result',{}).get('undone','?')}")
r7 = c.send("redo")
print(f"redo: {r7.get('result',{}).get('redone','?')}")

c.disconnect()
