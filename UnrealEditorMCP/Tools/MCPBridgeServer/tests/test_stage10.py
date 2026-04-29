import sys, json, time
sys.path.insert(0, r"E:\UEProject\UnrealEditorMCP\UnrealEditorMCP\Tools\MCPBridgeServer\src")
from mcp_bridge_server.bridge_client import UEBridgeClient
time.sleep(0.5)
c = UEBridgeClient()

print("=== get_mcp_config ===")
r = c.send("get_mcp_config")
print(f"actions: {r['result']['count']}")
for a in r["result"]["actions"]:
    print(f"  {a['action']} (cat={a['category']})")

print("\n=== Transaction state machine ===")
r2 = c.send("begin_transaction")
print(f"begin: ok={r2['ok']}")
r3 = c.send("begin_transaction")
print(f"double begin: ok={r3['ok']} err={r3.get('error',{}).get('code','?')}")
r4 = c.send("end_transaction")
print(f"end: ok={r4['ok']}")
r5 = c.send("end_transaction")
print(f"double end: ok={r5['ok']} err={r5.get('error',{}).get('code','?')}")

c.disconnect()
