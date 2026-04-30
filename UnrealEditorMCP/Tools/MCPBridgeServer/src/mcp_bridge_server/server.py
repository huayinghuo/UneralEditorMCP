"""
UE MCP Bridge Server
通过 stdio 传输层暴露 MCP 协议工具，将请求转发到 UE Editor 内的 MCP Bridge 插件
架构: MCP Client <--stdio--> UEMCPServer <--TCP--> UE Bridge Plugin

Tool 列表来源：C++ action registry (get_mcp_config) ∩ Python schema 注册表 (tool_schemas.py)
"""
import json
import asyncio
import logging

from mcp.server import Server
from mcp.server.stdio import stdio_server
import mcp.types as types

from .bridge_client import UEBridgeClient, UEBridgeError
from .tool_schemas import TOOL_SCHEMAS, BOOTSTRAP_MCP_CONFIG

logger = logging.getLogger(__name__)


class UEMCPServer:
    """MCP 协议服务器封装，动态生成 tool 列表"""

    def __init__(self, token=None):
        self._server = Server("ue-mcp-bridge")
        self._bridge = UEBridgeClient(token=token)
        self._cached_tools = None  # 缓存 UE 在线时生成的 tool 列表
        self._setup_handlers()

    def _setup_handlers(self):
        @self._server.list_tools()
        async def list_tools() -> list[types.Tool]:
            """动态生成 tool 列表：从 get_mcp_config 取 action ∩ 本地 schema 注册表"""
            actions = None
            online = False
            try:
                result = await asyncio.to_thread(self._bridge.send, "get_mcp_config")
                if result.get("ok"):
                    actions = result.get("result", {}).get("actions")
                    online = True
                else:
                    error = result.get("error", {})
                    error_code = error.get("code", "UNKNOWN") if isinstance(error, dict) else "UNKNOWN"
                    logger.warning("get_mcp_config returned error [%s]: %s",
                                   error_code,
                                   error.get("message", str(error)) if isinstance(error, dict) else str(error))
            except UEBridgeError as e:
                logger.debug("UE bridge offline for list_tools (code=%s): %s", e.code, e)
            except Exception:
                logger.debug("UE bridge offline for list_tools (unexpected error)", exc_info=True)

            if not actions:
                actions = BOOTSTRAP_MCP_CONFIG.get("actions", [])
                if not online:
                    logger.info("list_tools: UE offline, falling back to bootstrap config (%d actions)", len(actions))

            tools = []
            for entry in actions:
                action_name = entry.get("action")
                if not action_name:
                    continue
                schema = TOOL_SCHEMAS.get(action_name)
                if schema is None:
                    logger.warning("Action '%s' in registry but missing from Python TOOL_SCHEMAS (source=%s)",
                                   action_name, "ue-live" if online else "bootstrap")
                    continue
                tools.append(types.Tool(**schema))

            logger.info("Generated %d tools from %d actions (source=%s)",
                        len(tools), len(actions), "ue-live" if online else "bootstrap")
            self._cached_tools = tools
            return tools

        @self._server.call_tool()
        async def call_tool(name: str, arguments: dict) -> list[types.TextContent]:
            """MCP 协议：tools/call —— 统一参数透传到 UE Bridge"""
            try:
                # 去掉 ue_ 前缀映射为 UE action
                action = name.replace("ue_", "", 1)

                # 全部参数直接作为 payload 透传（_token 除外）
                payload = dict(arguments or {})

                result = await asyncio.to_thread(self._bridge.send, action, payload)

                if result.get("ok"):
                    response_data = result["result"]
                    if action == "get_mcp_config":
                        response_data["source"] = "ue-live"
                        response_data["connected"] = True
                        response_data["schema_registry_count"] = len(TOOL_SCHEMAS)
                    return [
                        types.TextContent(
                            type="text",
                            text=json.dumps(response_data, indent=2, ensure_ascii=False),
                        )
                    ]
                else:
                    error = result.get("error", {})
                    code = error.get("code", "UNKNOWN") if isinstance(error, dict) else "UNKNOWN"
                    msg = error.get("message", str(error)) if isinstance(error, dict) else str(error)
                    logger.warning("UE action '%s' returned error [%s]: %s", action, code, msg)
                    return [
                        types.TextContent(
                            type="text",
                            text=f"Error [{code}]: {msg}",
                        )
                    ]
            except UEBridgeError as e:
                logger.warning("UE bridge error for '%s': [%s] %s", name, e.code, e)
                if name == "ue_get_mcp_config":
                    # 保持 bootstrap 降级契约：仅 get_mcp_config 可离线返回
                    return [
                        types.TextContent(
                            type="text",
                            text=json.dumps(BOOTSTRAP_MCP_CONFIG, indent=2, ensure_ascii=False),
                        )
                    ]
                # 根据错误分类给出更精确的用户可读信息
                error_hints = {
                    "CONNECT_TIMEOUT": "UERuntimeError: Connection timed out. Is Unreal Editor running?",
                    "CONNECT_REFUSED": "UERuntimeError: Connection refused. Is the bridge plugin loaded?",
                    "CONNECT_FAILED": "UERuntimeError: Could not connect to UE Editor.",
                    "READ_TIMEOUT": "UERuntimeError: UE Editor is not responding in time.",
                    "PEER_CLOSED": "UERuntimeError: UE Editor closed the connection.",
                    "CLIENT_ALREADY_CONNECTED": "UERuntimeError: Another client is already connected (single-client model).",
                    "RESPONSE_MISMATCH": "UERuntimeError: Response ID mismatch — possible protocol desync.",
                    "CONNECTION_LOST": "UERuntimeError: Connection to UE Editor was lost.",
                }
                hint = error_hints.get(e.code, f"UE Editor is not available ({e.code}).")
                return [
                    types.TextContent(
                        type="text",
                        text=f"{hint}\nDetails: {e}",
                    )
                ]
            except Exception as e:
                logger.exception("Unexpected error in call_tool for '%s'", name)
                return [
                    types.TextContent(
                        type="text",
                        text=f"Internal error: {e}",
                    )
                ]

    async def run(self):
        async with stdio_server() as (reader, writer):
            await self._server.run(
                reader,
                writer,
                self._server.create_initialization_options(),
            )


def main():
    logging.basicConfig(level=logging.INFO)
    server = UEMCPServer()
    asyncio.run(server.run())


if __name__ == "__main__":
    main()
