# Last Operation

Session: 2026-04-30 10:36
Phase: 阶段 12A — 传输层稳态化与可观测性收口
Status: ✅ 完成，49 Handler（新增 1 个诊断查询）

## Slice A — 状态面补齐

- 新增 `FMCPGetBridgeRuntimeStatusHandler`（Read 类第 22 个，总计第 49 个 Handler）
  - 返回字段：`server_status` / `port` / `token_enabled` / `client_connected` / `last_error_code` / `last_error_message` / `transport_mode` / `bind_address`
  - 注册在 `MCPQueryHandlers.h/cpp`，通过 `FMCPBridgeServer*` 读取 server 状态
- `MCPBridgeServer` 新增 last error 追踪：
  - `SetLastError(Code, Message)` — 网络线程写入
  - `GetLastErrorCode()` / `GetLastErrorMessage()` — Game Thread 可读
  - `LastErrorCS` 保护字段线程安全
  - 所有 Run() 错误路径（SOCKET_SUBSYSTEM_FAILED / CREATE_SOCKET_FAILED / BIND_FAILED / LISTEN_FAILED）均记录 last error
- 模块启动 warning 日志结构化：含 `status=%d, port=%d, last_error=[%s] %s`
- `tool_schemas.py` 新增 `get_bridge_runtime_status` schema（ue_ 前缀映射）

## Slice B — 单客户端语义固定

- `Run()` 中 Accept 逻辑修改：
  - 无活跃客户端时正常 Accept
  - 已有活跃客户端时：Accept → 发送 `CLIENT_ALREADY_CONNECTED` 错误响应 → 关闭 Socket
  - 日志：`Warning: Rejected second client connection from ...`
- `MCPBridgeServer.h` 类注释写明单客户端独占模型
- `plan.md` / `plan_sync.md` 文档化单客户端限制

## Slice C — Python 错误诊断收口

- `bridge_client.py`：
  - 新增 `UEBridgeError(ConnectionError)` 基类，携带 `.code` 分类码
  - 错误分类：`CONNECT_TIMEOUT` / `CONNECT_REFUSED` / `CONNECT_FAILED` / `READ_TIMEOUT` / `PEER_CLOSED` / `CLIENT_ALREADY_CONNECTED` / `RESPONSE_MISMATCH` / `CONNECTION_LOST`
  - `connect()` 区分 timeout / refused / OSError
  - `_read_response()` 检测 `CLIENT_ALREADY_CONNECTED` 响应并上抛
  - 跳过不匹配响应时 debug 日志
- `server.py`：
  - `list_tools()` 在线失败时 warning 日志（含错误码）；离线时 info 级别注明 bootstrap fallback
  - `call_tool()` 根据 `UEBridgeError.code` 映射到差异化用户可读提示
  - bootstrap 降级契约不变（仅 `ue_get_mcp_config` 可离线返回）
  - 保持 `asyncio.to_thread()` 包装阻塞 I/O

## Slice D — 验收脚本

- 新建 `tests/test_stage12a.ps1`：
  - Part 1: ping / get_bridge_runtime_status / get_mcp_config 基本联通
  - Part 2: Runtime Status 在 Connected / Listening 两种状态下字段正确
  - Part 3: 错误路径（非法 JSON→PARSE_ERROR、缺失 action→MISSING_ACTION、未知 action）
  - Part 4: 单客户端独占（第二连接被拒绝 + 第一客户端不受影响）
  - Part 5: 断连恢复（disconnect→Listening→新连接可用）
  - 含 PASS/FAIL 计数和断言机制

## 变更文件清单（8 个文件）

| 文件 | 变更类型 |
|------|----------|
| `Public/MCPBridgeServer.h` | 新增字段/方法/注释 |
| `Private/MCPBridgeServer.cpp` | last error 记录 + 第二连接拒绝 + 新 Handler 注册 |
| `Public/Handlers/MCPQueryHandlers.h` | 新增 Handler 类声明 |
| `Private/Handlers/MCPQueryHandlers.cpp` | 新增 Handler 实现 |
| `Private/UnrealEditorMCPBridgeModule.cpp` | startup warning 结构化 |
| `Tools/.../bridge_client.py` | UEBridgeError 错误分类 |
| `Tools/.../server.py` | 日志增强 + 错误提示映射 |
| `Tools/.../tool_schemas.py` | 新增 schema |
