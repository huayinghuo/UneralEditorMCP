import sys, json, time
sys.path.insert(0, r"E:\UEProject\UnrealEditorMCP\UnrealEditorMCP\Tools\MCPBridgeServer\src")
from mcp_bridge_server.bridge_client import UEBridgeClient

time.sleep(0.3)
c = UEBridgeClient()
bp_path = "/Game/Blueprints/TestBP_MCP.TestBP_MCP"

print("=== Add Variable ===")
r = c.send("blueprint_add_variable", {"asset_path": bp_path, "var_name": "Health", "var_type": "float", "default_value": "100.0"})
print(json.dumps(r, indent=2, ensure_ascii=False))

print("\n=== After Add Var ===")
r2 = c.send("get_blueprint_info", {"asset_path": bp_path})
print(f"variables: {len(r2['result']['variables'])}")

print("\n=== Add Function ===")
r3 = c.send("blueprint_add_function", {"asset_path": bp_path, "func_name": "DoSomething"})
print(json.dumps(r3, indent=2, ensure_ascii=False))

print("\n=== After Add Func ===")
r4 = c.send("get_blueprint_info", {"asset_path": bp_path})
print(f"functions: {len(r4['result']['functions'])}")

c.disconnect()
