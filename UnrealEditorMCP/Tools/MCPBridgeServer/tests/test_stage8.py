import sys, json
sys.path.insert(0, r"E:\UEProject\UnrealEditorMCP\UnrealEditorMCP\Tools\MCPBridgeServer\src")
from mcp_bridge_server.bridge_client import UEBridgeClient
c = UEBridgeClient()

print("=== Screenshot ===")
r = c.send("viewport_screenshot")
print(json.dumps(r, indent=2, ensure_ascii=False))

print("\n=== List Engine Materials ===")
r2 = c.send("list_materials", {"path": "/Engine"})
result = r2.get("result", {})
print(f"count: {result.get('count', '?')}")
for m in result.get("materials", [])[:4]:
    print(f"  {m['name']} ({m['class']})")

if result.get("materials"):
    first = result["materials"][0]["path"]
    print(f"\n=== Get Material Info ({first}) ===")
    r3 = c.send("get_material_info", {"asset_path": first})
    info = r3.get("result", {})
    print(f"  class: {info.get('class')}")
    print(f"  is_instance: {info.get('is_instance')}")
    if "blend_mode" in info:
        print(f"  blend_mode: {info.get('blend_mode')}")
    print(f"  scalar_params: {len(info.get('scalar_params',[]))}")
    print(f"  vector_params: {len(info.get('vector_params',[]))}")
    print(f"  texture_params: {len(info.get('texture_params',[]))}")

c.disconnect()
