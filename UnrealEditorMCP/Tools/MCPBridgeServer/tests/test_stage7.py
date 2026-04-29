import sys, json
sys.path.insert(0, r"E:\UEProject\UnrealEditorMCP\UnrealEditorMCP\Tools\MCPBridgeServer\src")
from mcp_bridge_server.bridge_client import UEBridgeClient
c = UEBridgeClient()

# Test rotation semantics
print("=== Rotation ===")
r = c.send("spawn_actor", {"class_name": "PointLight", "rotation": {"pitch": 30, "yaw": 45, "roll": 10}})
print(f"spawn: {r['ok']}, rot={r['result']['transform']['rotation']}")

# Test undo without transaction
print("\n=== Undo (no tx) ===")
r2 = c.send("undo")
print(f"ok={r2['ok']}, err={r2.get('error',{}).get('code','?')}")

# Test undo after transaction
print("\n=== Transaction + undo ===")
c.send("begin_transaction", {"description": "test"})
c.send("spawn_actor", {"class_name": "PointLight"})
c.send("end_transaction")
r3 = c.send("undo")
print(f"undo 1: ok={r3['ok']}")
r4 = c.send("undo")
print(f"undo 2: ok={r4['ok']}, err={r4.get('error',{}).get('code','?')}")

# Test property found/not found
print("\n=== Property found semantics ===")
r5 = c.send("level_get_actor_property", {"actor_name": "NonExistentActorXYZ", "property_name": "bHidden"})
print(f"non-exist actor: ok={r5['ok']}, err={r5.get('error',{}).get('code','?')}")

c.disconnect()
