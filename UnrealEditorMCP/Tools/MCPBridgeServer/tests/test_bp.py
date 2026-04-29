import sys, json
sys.path.insert(0, r"E:\UEProject\UnrealEditorMCP\UnrealEditorMCP\Tools\MCPBridgeServer\src")
from mcp_bridge_server.bridge_client import UEBridgeClient
c = UEBridgeClient()

print("=== List Blueprints ===")
r = c.send("list_blueprints")
print(f"count: {r['result']['count']}")
for bp in r["result"]["blueprints"][:3]:
    print(f"  {bp['name']}")

print("\n=== Create Blueprint ===")
r2 = c.send("create_blueprint", {"name": "TestBP_MCP", "parent_class": "Actor"})
print(json.dumps(r2, indent=2, ensure_ascii=False))

if r2["ok"]:
    bp_path = r2["result"]["path"]
    print(f"\n=== Get Blueprint Info ({bp_path}) ===")
    r3 = c.send("get_blueprint_info", {"asset_path": bp_path})
    print(json.dumps(r3, indent=2, ensure_ascii=False))

print("\n=== List Blueprints (after create) ===")
r4 = c.send("list_blueprints")
print(f"count: {r4['result']['count']}")

c.disconnect()
