import sys
sys.path.insert(0, r"E:\UEProject\UnrealEditorMCP\UnrealEditorMCP\Tools\MCPBridgeServer\src")

import asyncio
import json
from mcp_bridge_server.server import UEMCPServer


def test_via_client():
    """Test the MCP server tools by calling list_tools directly"""
    server = UEMCPServer()
    print("MCP Server created OK")

    # The list_tools handler is registered on the server
    # We can call it directly since we have access to the internal server
    async def do_test():
        # Call the registered list_tools handler
        tools_result = await server._server._mcp_server._tool_handler.list_tools()
        # Try alternate API
        return tools_result

    try:
        result = asyncio.run(do_test())
        print(f"Result: {result}")
    except AttributeError as e:
        print(f"API attempt failed: {e}")

        # Try inspecting the server to find tools
        print("\nServer attributes:")
        for attr in dir(server._server):
            if not attr.startswith("__"):
                print(f"  {attr}")


if __name__ == "__main__":
    test_via_client()
