# Unreal Editor MCP Bridge 开发计划

> 详细规划见外部文件：`C:\Users\萤火\.local\share\kilo\plans\1777390210726-swift-squid.md`

## 实施状态（2026-04-30）

| 阶段 | 内容 | 状态 |
|------|------|------|
| 0-4 | MVP 初始化 | ✅ |
| 5 | 联调验证 | ✅ |
| 6 | C++ 执行层重构（Dispatcher + Handler + Helpers） | ✅ |
| 6+ | Handler 按领域拆分 | ✅ |
| 7 | 写命令 + 稳定性收敛（7 项修复） | ✅ |
| P0/P1 | component + 资产 + 事务 | ✅ |
| 8 | BP/Material/Screenshot/Widget/Dirty 域能力 | ✅ |
| 9 | 安全权限体系（三级分类 + token + 审计） | ✅ |
| 10 | 契约收敛（First Pass）：分类修正 / ok 语义 / 事务状态机 / action registry | ✅ |
| 11 | 深层资产编辑（Material 写入 + BP 变量/函数，5 Handler） | ✅ |
| 10 Second Pass | 失败语义 / 资产冲突 / 断连事务 / 配置增强 | ✅ |
| 10B | **治理闭环**（ini 配置 / get_mcp_config 端到端 / 队列丢弃 / 写语义统一 / BP 重名检测） | ✅ |
| 11A | **受控 Blueprint 图编辑**（6 Handler：创建 Actor BP / EventGraph 信息 / 事件节点 / CallFunction 节点 / 连线 / 编译保存） | ✅ |
| 13 | **仓库治理**（.gitignore / .editorconfig / .gitattributes / README.md） | ✅ |
| **10C** | **Python 侧自举配置收敛**（ue_get_mcp_config 自举 / 在线覆盖 / 断连退化 / list_tools 统一配置源） | ✅ |
| **11B** | **声明式 Blueprint 图重建**（blueprint_apply_spec / blueprint_export_spec，spec → nodes + edges → BP） | ✅ |
| **12A** | **传输层稳态化与可观测性收口**（runtime status 诊断 + 单客户端策略固定 + Python 错误分类 + 验收脚本） | ✅ |
| **14** | **MCP Resources 知识层**（4 static + 2 live resources） | ✅ |
| **14A** | **Resources 契约统一 + 测试分层** | ✅ |
| **15A** | **BridgeClient 并发串行化** | ✅ |
| **16** | **Widget 能力完整深化**（50→58 Handler，11 新增 + 2 增强，26/26 验收通过） | ✅ |

## Handler 清单（58 个）

```
Read (26): ping / get_bridge_runtime_status / editor_info / project_info / world_state
           list_assets / get_asset_info / get_mcp_config / selected_actors
           level_actors / get_actor_property / get_component_property
           list_blueprints / get_bp_info / blueprint_get_event_graph_info
           blueprint_export_spec / list_materials / get_material_info
           list_widgets / get_widget_info / viewport_screenshot / get_dirty_packages
           widget_get_property_schema / widget_get_slot_schema / widget_find

Write (31): spawn_actor / set_transform / actor_set_property / save_level
            delete_actor / set_component_property / begin/end/undo/redo
            create_blueprint / blueprint_add_variable / blueprint_add_function
            blueprint_add_component / blueprint_create_actor_class
            blueprint_add_event_node / blueprint_add_call_function_node
            blueprint_connect_pins / blueprint_compile_save / blueprint_apply_spec
            create_widget_bp / widget_add_child / widget_remove_child
            widget_set_property / widget_set_root / widget_reparent
            widget_reorder_child / widget_rename / widget_set_slot_property
            widget_duplicate / widget_wrap_with_panel
            material_set_scalar/vector/texture_param

Dangerous (1): execute_python_snippet
```

## Stage 16 验收测试（2026-04-30）

| 测试域 | 项目 | 结果 |
|--------|------|------|
| Part 1 | 创建含 root CanvasPanel 的 Widget BP | ✅ |
| Part 2 | 构建 5 节点树（VBox/TextBlock/Image/Button/TextBlock） | ✅ |
| Part 3 | 查询树、find、property_schema、slot_schema | ✅ |
| Part 4 | 属性编辑（Text）+ Slot 编辑（Padding/HAlign） | ✅ |
| Part 5 | Reparent, Rename, Duplicate | ✅ |
| Part 6 | Wrap（Border 包裹 Button） | ✅ |
| Part 7 | 错误路径（ROOT_ALREADY_EXISTS/CYCLE/CONFLICT/NOT_FOUND/SLOT） | ✅ |
| Part 8 | 编译保存 + 终态验证 | ✅ |
| **总计** | **26/26 PASS, 0 FAIL** | ✅ |

## 11A 验收测试（2026-04-30）

| 测试# | 命令 | ok | 关键结果 |
|-------|------|-----|----------|
| 1 | `ping` | ✅ | 桥接联通 |
| 2 | `blueprint_create_actor_class` (name=TestBPA_Stage11A) | ✅ | asset_path=/Game/Blueprints/TestBPA_Stage11A |
| 3 | `blueprint_get_event_graph_info` | ✅ | exists=True, event_node_count=3 |
| 4 | `blueprint_add_event_node` (ReceiveBeginPlay) | ✅ | node_guid 返回，created=False（已有，正确复用） |
| 5 | `blueprint_add_call_function_node` (PrintString) | ✅ | node_guid 返回 |
| 6 | `blueprint_connect_pins` (BeginPlay→PrintString) | ✅ | connected=True |
| 7 | `blueprint_compile_save` (save=true) | ✅ | compiled=True, saved=True |
| 8 | 错误参数：parent_class=NotAnActorClass | ✅ ok=False | CLASS_NOT_FOUND |
| 9 | 错误参数：function_name=NonExistent | ✅ ok=False | FUNCTION_NOT_FOUND |
| 10 | 错误参数：无效 GUID | ✅ ok=False | NODE_NOT_FOUND |

## 10B 治理闭环验收

| 领域 | 关键交付 | 状态 |
|------|----------|------|
| 权限收敛 | `DefaultUnrealEditorMCPBridge.ini` → `StartupModule` 读取 Token/Port | ✅ |
| 契约化 | `get_mcp_config` 返回 action/category/mode/version | ✅ |
| 消除双写 | `tool_schemas.py`(37 schema) + `server.py` 动态取 `get_mcp_config ∩ TOOL_SCHEMAS` | ✅ |
| 事务收敛 | 状态机 + 断连 auto-end + RequestQueue 丢弃 | ✅ |
| 资产一致性 | SAVE_FAILED / ASSET_CONFLICT / PARAM_NOT_FOUND / DUPLICATE_* | ✅ |
| 写语义统一 | 6 类命令 ok=false 覆盖所有失败路径 | ✅ |

## 统一错误码体系

| 错误码 | 触发条件 | 命令 |
|--------|----------|------|
| MISSING_PARAM | 必需参数缺失 | 全部 |
| UNKNOWN_ACTION | action 未注册 | Dispatch 层 |
| TOKEN_REQUIRED | 受限模式下 token 缺失 | Dispatch 层 |
| PYTHON_EXEC_FAILED | Python 执行异常 | execute_python_snippet |
| PYTHON_UNAVAILABLE | PythonScriptPlugin 未启用 | execute_python_snippet |
| NO_WORLD | 无可用编辑器世界 | Actor 类 |
| ACTOR_NOT_FOUND | Actor 名称不存在 | Actor 类 |
| COMPONENT_NOT_FOUND | Component 名称不存在 | Actor 类 |
| SPAWN_FAILED / DELETE_FAILED | 创建/删除失败 | 写操作类 |
| BP_NOT_FOUND / WIDGET_NOT_FOUND | Blueprint/Widget 不存在 | Blueprint/Widget |
| CLASS_NOT_FOUND | 父类未找到 | create_blueprint |
| CREATE_FAILED | 资产创建框架失败 | create_* |
| SAVE_FAILED | 保存到磁盘失败 | create_* / save_current_level |
| ASSET_CONFLICT | 重名资产 | create_* |
| MATERIAL_NOT_FOUND | Material 不存在 | material_* |
| PARAM_NOT_FOUND | 参数不存在 | material_set_* |
| DUPLICATE_VARIABLE | 同名变量已存在 | blueprint_add_variable |
| DUPLICATE_FUNCTION | 同名函数已存在 | blueprint_add_function |
| NAME_CONFLICT | 变量名占用函数名 | blueprint_add_function |
| UNSUPPORTED_TYPE | 不支持的变量类型 | blueprint_add_variable |
| TRANSACTION_ACTIVE | 已有活跃事务 | begin_transaction |
| NO_ACTIVE_TRANSACTION | 无活跃事务 | end_transaction |
| UNDO_FAILED / REDO_FAILED | 撤销/重做失败 | undo / redo |
| PROPERTY_SET_FAILED | 属性写入失败 | set_property 类 |
| READ_FAILED / NO_VIEWPORT | 截图失败 | viewport_screenshot |
| PARSE_ERROR | JSON 解析失败 | 传输层 |
| GRAPH_NOT_FOUND | Blueprint 无 EventGraph | blueprint_add_event_node / blueprint_add_call_function_node / blueprint_connect_pins |
| FUNCTION_NOT_FOUND | 函数未找到 | blueprint_add_call_function_node |
| FUNCTION_NOT_CALLABLE | 函数非 BlueprintCallable | blueprint_add_call_function_node |
| NODE_NOT_FOUND | node_guid 对应节点不存在 | blueprint_connect_pins |
| PIN_NOT_FOUND | pin_name 对应引脚不存在 | blueprint_connect_pins |
| SCHEMA_NOT_FOUND | Graph Schema 为空 | blueprint_connect_pins |
| COMPILE_FAILED | 编译后 Status == BS_Error | blueprint_compile_save |
| SOCKET_SUBSYSTEM_FAILED | 无法获取平台 Socket 子系统 | Server Run() |
| CREATE_SOCKET_FAILED | 创建监听 Socket 失败 | Server Run() |
| BIND_FAILED | 绑定端口失败 | Server Run() |
| LISTEN_FAILED | 开始监听失败 | Server Run() |
| CLIENT_ALREADY_CONNECTED | 第二客户端被拒绝（单客户端独占模型） | Server Accept |
| CONNECT_TIMEOUT | Python 客户端连接超时 | bridge_client.py |
| CONNECT_REFUSED | Python 客户端连接被拒绝 | bridge_client.py |
| READ_TIMEOUT | Python 客户端读取超时 | bridge_client.py |
| PEER_CLOSED | UE 侧关闭连接 | bridge_client.py |
| RESPONSE_MISMATCH | 响应 ID 不匹配（协议串包） | bridge_client.py |

---

## 阶段 10C：Python 侧自举配置收敛（✅ 已完成）

### 目标

让 Python MCP Server 在 UE 未连接时仍能稳定暴露 `ue_get_mcp_config`，UE 上线后自动切换为 live config，断连后退化回 bootstrap config。

### 当前问题

- `list_tools()` 离线时返回仅含 `ue_get_mcp_config` 的占位 tool
- `call_tool("ue_get_mcp_config")` 离线时返回 ConnectionError（非结构化 config）
- 两个路径的离线退化结果来源不一致

### 三层配置模型

#### 1. Bootstrap Config（`BOOTSTRAP_MCP_CONFIG`）

文件：`tool_schemas.py`

```python
BOOTSTRAP_MCP_CONFIG = {
    "version": "1.0.0",
    "bridge_protocol": "TCP/JSON Lines",
    "source": "python-bootstrap",
    "connected": False,
    "ue_version": None,
    "mode": "development",
    "token_enabled": False,
    "actions": [
        {"action": "get_mcp_config", "category": "Read", "mode": "development"},
    ],
    "count": 1,
}
```

设计约束：
- `source = "python-bootstrap"` 显式区分在线/离线
- `connected = False` 明确当前状态
- `actions` 最小化（仅 `get_mcp_config`），不伪装成 UE 全量 registry
- `TOOL_SCHEMAS` 依然为此 bootstrap actions 提供 schema 查询

#### 2. Live Override（UE 在线时）

文件：`server.py` — `call_tool("ue_get_mcp_config")`

```python
# 在线路径（当前行为保持不变）
result = self._bridge.send("get_mcp_config")
# 在返回结果中补充 Python 侧元数据
live_config = result["result"]
live_config["source"] = "ue-live"
live_config["connected"] = True
live_config["schema_registry_count"] = len(TOOL_SCHEMAS)
```

#### 3. Disconnect Degradation（UE 断连时）

文件：`server.py` — `list_tools()` 和 `call_tool()`

```python
# list_tools() 离线：统一使用 BOOTSTRAP_MCP_CONFIG["actions"]
actions = BOOTSTRAP_MCP_CONFIG["actions"]
for entry in actions:
    schema = TOOL_SCHEMAS.get(entry["action"])
    if schema: tools.append(types.Tool(**schema))

# call_tool("ue_get_mcp_config") 离线：返回 bootstrap config
if name == "ue_get_mcp_config":
    return json.dumps(BOOTSTRAP_MCP_CONFIG)
```

### 统一配置源约束

- `list_tools()` 离线结果 ∩ `call_tool("ue_get_mcp_config")` 离线结果 **必须来自同一个 `BOOTSTRAP_MCP_CONFIG`**
- `list_tools()` 在线结果 ∩ `call_tool("ue_get_mcp_config")` 在线结果 **必须来自同一个 UE live response**
- bootstrap 不承诺与 UE 在线结果一致，仅反映 Python 本地已知的最小事实

### 状态切换流程

```
                    UE TCP 可达？
                   /           \
              是 → live         否 → bootstrap
                    |                 |
        list_tools  ← C++ action ∩   ← BOOTSTRAP_MCP_CONFIG
                       TOOL_SCHEMAS     ∩ TOOL_SCHEMAS
                    |
        call_tool   → 转发到 UE C++   → "ue_get_mcp_config":
                    |                    返回 BOOTSTRAP_MCP_CONFIG
                    |                  → 其他 tool:
                    |                    ConnectionError
```

### 实现切分（最小改动）

| 文件 | 变更 | 说明 |
|------|------|------|
| `tool_schemas.py` | 新增 `BOOTSTRAP_MCP_CONFIG` | 离线基础配置 |
| `server.py` — `list_tools()` | 离线分支改用 `BOOTSTRAP_MCP_CONFIG["actions"]` | 统一配置源 |
| `server.py` — `call_tool()` | 对 `ue_get_mcp_config` 离线特判 | 返回 bootstrap 而非 ConnectionError |

### 10C 验收标准

- [x] UE 不在线时 `list_tools()` 返回包含 `ue_get_mcp_config` 的合法 tool 列表
- [x] UE 不在线时 `call_tool("ue_get_mcp_config")` 返回结构化 JSON（非 ConnectionError）
- [x] `ue_get_mcp_config` 离线返回 `source="python-bootstrap"`, `connected=false`
- [x] UE 在线后 `ue_get_mcp_config` 返回 `source="ue-live"`, `connected=true`
- [x] UE 断连后 `list_tools()` 和 `call_tool("ue_get_mcp_config")` 退化结果一致
- [x] 其他 tool 离线时仍返回 ConnectionError（不变化）

### 10C 变更文件（4 个）

| 文件 | 变更 |
|------|------|
| `tool_schemas.py` | 新增 `BOOTSTRAP_MCP_CONFIG`（+18 行）；docstring 更新 |
| `server.py` — import | 新增 `BOOTSTRAP_MCP_CONFIG` 导入 |
| `server.py` — `list_tools()` | 离线分支改为从 `BOOTSTRAP_MCP_CONFIG["actions"]` 生成 tools |
| `server.py` — `call_tool()` | `ue_get_mcp_config` 离线特判返回 bootstrap JSON；在线增强 `source=ue-live` |
| `Tests/test_stage11a.ps1` | 添加时间戳唯一名称避免重复资产冲突 |

### 明确不做

- 不在 Python 侧完整复制 C++ action registry
- 不引入持久缓存或磁盘快照机制
- 不修改 `bridge_client.py` / `UEBridgeClient` 底层通信逻辑
- 不修改 C++ 侧代码

---

## 阶段 11B：声明式 Blueprint 图重建（✅ 已完成）

### 与 10C 的衔接关系

**执行顺序**: 10C →（通过验收）→ 11B

11A 已提供 6 个受控图编辑命令（create / event / call / connect / compile）。11B 在其上构建声明式重建能力。

### 11B 推荐交付

| # | Action | 类别 | 说明 |
|---|--------|------|------|
| 1 | `blueprint_apply_spec` | Write | 输入声明式 spec（{blueprint, graphs[], events[], nodes[], edges[], defaults}），创建/更新 BP 图 |
| 2 | `blueprint_export_spec` | Read | 将已有 BP 导出为 spec（方便 round-trip / diff） |

### 11B 风险控制

- 第一版仅支持 **Actor Blueprint + EventGraph**
- 第一版仅支持一小组白名单节点（事件节点 + CallFunction 节点）
- 第一版仅支持有限 pin 类型
- 第一版不做节点删除和全量 graph 覆盖，先做增量构建

### 11B 文件拆分

| 文件 | 内容 |
|------|------|
| `Public/Handlers/MCPBlueprintSpecHandlers.h` | Handler 声明 |
| `Private/Handlers/MCPBlueprintSpecHandlers.cpp` | 执行逻辑 |
| `Public/MCPBlueprintSpecTypes.h` | Spec 类型定义（FBlueprintGraphSpec 等） |

### 11B 验收标准

- [x] 能从 spec JSON 创建 Actor BP 并填充 EventGraph 结构
- [x] 能从已有 BP 导出 spec（含 node/pin/edge 信息）
- [x] spec round-trip: apply → export → re-apply 边格式兼容性验证通过（`from_node`/`to_node` spec ID 一致）
- [x] 每步有结构化错误码

---

## 阶段 12A：传输层稳态化与可观测性收口（✅ 已完成）

### 目标

在不重写传输协议的前提下，把当前 `localhost TCP JSON Lines + 外部 Python MCP Server` 方案补成更稳定、更可诊断的闭环。

### 明确范围

**做**：
1. 补齐运行时状态可见性（`get_bridge_runtime_status` action）
2. 收敛连接失败/启动失败/断连恢复的语义（`LastErrorCode/LastErrorMessage`）
3. 固定单客户端独占模型，第二连接被显式拒绝（`CLIENT_ALREADY_CONNECTED`）
4. Python 侧连接错误分类收敛（`UEBridgeError` 携带分类码）
5. PowerShell 验收脚本覆盖在线/离线/断连/重复连接场景

**不做**：
- 不新增大批业务 Handler（仅新增 1 个诊断查询）
- 不切换 HTTP/SSE/WebSocket
- 不实现 UE 侧真正多客户端并发会话池
- 不做远程部署、TLS、LAN 暴露

### 核心交付

| # | 交付物 | 文件 |
|---|--------|------|
| 1 | `get_bridge_runtime_status` Handler | C++: `MCPQueryHandlers.h/cpp` / Python: `tool_schemas.py` |
| 2 | `SetLastError` / `GetLastErrorCode` / `GetLastErrorMessage` | `MCPBridgeServer.h/cpp` |
| 3 | 单客户端拒绝策略（`CLIENT_ALREADY_CONNECTED`） | `MCPBridgeServer.cpp:222-241` |
| 4 | Python `UEBridgeError` 错误分类（CONNECT_TIMEOUT / READ_TIMEOUT / PEER_CLOSED 等） | `bridge_client.py` |
| 5 | 模块 startup warning 结构化（含 status + last error） | `UnrealEditorMCPBridgeModule.cpp` |
| 6 | `server.py` list_tools/call_tool 日志增强 | `server.py` |
| 7 | 阶段 12A 验收脚本 | `tests/test_stage12a.ps1` |

### 运行时诊断字段

```json
{
  "server_status": "Listening|Connected|Error|Stopped|Unstarted",
  "port": 9876,
  "token_enabled": false,
  "client_connected": true,
  "last_error_code": "BIND_FAILED",
  "last_error_message": "Failed to bind to 127.0.0.1:9876",
  "transport_mode": "tcp-jsonlines",
  "bind_address": "127.0.0.1"
}
```

### 单客户端模型

- 同一时刻只允许一个 TCP 客户端连接
- 第二客户端到来时：Accept → 发送 `CLIENT_ALREADY_CONNECTED` 错误 → 关闭 Socket
- 日志：`Warning: Rejected second client connection from ...`
- Python 客户端收到 `CLIENT_ALREADY_CONNECTED` 后抛出 `UEBridgeError`

### 12A 变更文件（8 个）

| 文件 | 变更 |
|------|------|
| `Public/MCPBridgeServer.h` | 新增 `IsClientConnected` / `GetServerPort` / `SetLastError` / `GetLastError*`；新增 `LastErrorCode/Message` + `LastErrorCS` 字段；更新类注释 |
| `Private/MCPBridgeServer.cpp` | 所有错误路径记录 last error；第二客户端拒绝逻辑；注册 `FMCPGetBridgeRuntimeStatusHandler`；`GetLastError*` 实现 |
| `Public/Handlers/MCPQueryHandlers.h` | 新增 `FMCPGetBridgeRuntimeStatusHandler` 类声明 |
| `Private/Handlers/MCPQueryHandlers.cpp` | 实现 `FMCPGetBridgeRuntimeStatusHandler::Execute()` |
| `Private/UnrealEditorMCPBridgeModule.cpp` | 启动 warning 日志结构化（含 status/port/last_error） |
| `Tools/.../bridge_client.py` | `UEBridgeError` 基类 + 连接/读取/对端关闭错误分类；`CLIENT_ALREADY_CONNECTED` 检测 |
| `Tools/.../server.py` | `list_tools` / `call_tool` 日志增强；`UEBridgeError` 错误码到用户可读提示映射 |
| `Tools/.../tool_schemas.py` | 新增 `get_bridge_runtime_status` schema |

### 验收标准

- [x] `get_bridge_runtime_status` 返回 `server_status` / `port` / `token_enabled` / `client_connected` / `last_error_*` / `transport_mode` / `bind_address`
- [x] 启动失败时 `last_error_code` 为非空（如 `BIND_FAILED`），不依赖 UE 日志判断
- [x] 单客户端：第二连接被拒绝，返回 `CLIENT_ALREADY_CONNECTED` 错误
- [x] Python 侧 `UEBridgeError` 携带分类码（`CONNECT_TIMEOUT` / `READ_TIMEOUT` / `PEER_CLOSED` 等）
- [x] `server.py` 错误提示根据分类码给出差异化用户可读信息
- [x] `list_tools` 在线/离线切换有 debug 日志
- [x] 不破坏现有 10C / 11A / 11B 行为
- [x] PowerShell 验收脚本覆盖在线/离线/断连/重复连接 4 类场景 + 至少 3 条错误路径

---

## 修复任务追踪（Review Findings 对照）

### 一次 Review（§4.2）— 5 项

| # | 发现 | 严重度 | 状态 | 证据 |
|---|------|--------|------|------|
| 1 | 协议关联与重连正确性不足 | P1 | ⚠️ 部分 | Python `bridge_client.py:91` ID 校验 ✅；UE 侧单全局 socket ⚠️（架构限制，单连接模式） |
| 2 | 服务启动健康状态不可见 | P1 | ✅ 阶段 12A | `SetLastError()` 记录 Bind/Listen 错误；`get_bridge_runtime_status` 暴露 last_error；模块启动轮询含结构化 warning |
| 3 | Python MCP async 内部阻塞 socket I/O | P1 | ✅ | `server.py:76` — `asyncio.to_thread()` 将 I/O 卸到线程池 |
| 4 | 反射属性与事务语义不可靠 | P1 | ✅ | `MCPBridgeHelpers.cpp:118-120` — Modify() 先于 ImportText；返回值检查 |
| 5 | transform/transaction 语义不一致 | P2 | ⚠️ 已知 | Rotation 映射 `(Y,Z,X)` → 约定（`MCPBridgeHelpers.h:31`）；undo/redo 已修复 |

### 二次 Review（§4.4）— 6 项

| # | 发现 | 严重度 | 状态 | 证据 |
|---|------|--------|------|------|
| 6 | 权限模型默认未生效 | Critical | ✅ 设计 | 开发模式下 token 为空 → 不校验（INI `Token=`），受限模式需显式配置 |
| 7 | viewport_screenshot 归类为 Read 但有写副作用 | High | ✅ | `MCPViewportHandlers.h:11` — 已改为 Write（注释："写磁盘，非纯读"） |
| 8 | execute_python_snippet 失败仍返回 ok=true | High | ✅ | `MCPPythonHandler.cpp:26-31` — 检查 `ExecPythonCommand` 返回值，失败返回 false |
| 9 | 事务生命周期未与连接/会话绑定 | High | ✅ | `MCPBridgeServer.cpp:223` — 断连调用 `MCPTransaction_AutoEndIfActive()` |
| 10 | C++ Handler 与 Python tools 手工双写 | Medium | ✅ | `server.py:33-59` — `get_mcp_config` ∩ `TOOL_SCHEMAS` 动态生成 |
| 11 | 资产创建保存失败仍返回成功 | Medium | ✅ | `MCPBlueprintHandlers.cpp:208-211` / `MCPWidgetHandlers.cpp:159-162` — 保存失败返回 false + SAVE_FAILED |

### 三次 Review（§4.5）— 5 项

| # | 发现 | 严重度 | 状态 | 证据 |
|---|------|--------|------|------|
| 12 | get_mcp_config 未在 MCP 层暴露 | Medium | ✅ | 10B 已暴露 + 10C 添加离线自举（`BOOTSTRAP_MCP_CONFIG`） |
| 13 | Token 默认运行链路无配置入口 | Medium | ✅ 设计 | INI 文件提供 Token/Port 配置入口，开发模式默认不启用 |
| 14 | 断连仅清 ResponseQueue，未清 RequestQueue | Medium | ✅ | `MCPBridgeServer.cpp:239-241` — 两端队列均已清空 |
| 15 | material_set_* 静默成功 | Medium | ✅ | `MCPMaterialHandlers.cpp:179-182` 等 — 参数不存在返回 false + PARAM_NOT_FOUND |
| 16 | 单命令已收敛，全系统未统一 | Medium | ✅ | 统一切换到 `BuildErrorResponse` 模式；Stop() 重入已修；43→48 Handler 全部遵循 |

### 本轮修复（阶段 11A-11B 期间）

| # | 问题 | 修复 |
|---|------|------|
| 17 | `GetMemberName()` UE 5.3 API 不兼容 | `FName Name = Ref.GetMemberName();`（返回值，非 out 参数） |
| 18 | Stop() 重入导致栈溢出 | `MCPBridgeServer.cpp:109` — `if (bStopping) return;` 防重入守卫 |
| 19 | 新 Handler 缺少 `BP->Modify()` | 3 处补全（event_node / call_function / connect_pins） |
| 20 | export_spec 边格式与 apply_spec 不兼容 | GUID → spec ID 映射（`GuidToSpecId`），统一 `from_node`/`to_node` 字段 |

### 待处理（P1/P2，非阻塞）

| # | 项目 | 说明 |
|---|------|------|
| A | ~~服务启动健康状态~~ | ✅ 阶段 12A 已修复：`SetLastError()` / `GetLastErrorCode/Message()` / `get_bridge_runtime_status` 诊断接口 |
| B | ~~UE 侧多连接支持~~ | ✅ 阶段 12A 已收敛：明确定义为单客户端独占模型，第二连接被拒绝并返回 `CLIENT_ALREADY_CONNECTED` |
| C | Rotation 映射统一 | 当前 `FRotator(Y, Z, X)` 约定文档化，可考虑统一为 `{pitch, yaw, roll}` 参数名 |
