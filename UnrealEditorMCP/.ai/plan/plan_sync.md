# Unreal Editor MCP Bridge — 协作计划摘要

> 外部权威主计划：`C:\Users\萤火\.local\share\kilo\plans\1777390210726-swift-squid.md`
> 本文件为主计划的精简协作镜像，仅供远端子仓库/新机器/新 worktree 与其它 agent 获取共享上下文。
> 同步规则：**外部主计划先更新，本文件在阶段切换或关键决策变化时单向同步。**

---

## 当前阶段（2026-04-30）

**当前主线：阶段 14A 已完成** — Resources 知识层交付 + 契约统一 + 双模测试

### 已完成里程碑

| 阶段 | 状态 |
|------|------|
| 0-4 MVP 初始化 | ✅ |
| 5 联调验证 | ✅ |
| 6 执行层重构（Dispatcher + Handler） | ✅ |
| 7 写命令 + 稳定性收敛 | ✅ |
| 8 BP/Material/Screenshot/Widget/Dirty | ✅ |
| 9 安全权限体系（三级分类 + token） | ✅ |
| 10 契约收敛 First Pass | ✅ |
| 11 深层资产编辑（Material + BP 变量/函数） | ✅ |
| 10 Second Pass（失败语义 / 断连事务） | ✅ |
| 10B 治理闭环（ini 配置 / get_mcp_config 端到端） | ✅ |
| 11A 受控 Blueprint 图编辑（6 Handler） | ✅ |
| 13 仓库治理（.gitignore / README.md） | ✅ |
| 10C Python 自举配置收敛（断连退化） | ✅ |
| 11B 声明式 Blueprint 图重建（apply_spec / export_spec） | ✅ |
| 12A 传输层稳态化与可观测性收口 | ✅ |
| 14 MCP Resources 知识层（Static + Live） | ✅ |
| 14A Resources 契约统一 + 测试分层 | ✅ |
| 15A BridgeClient 并发串行化 + 镜像收口 | ✅ |

### 当前不在做的

- 不新增大批业务 Handler（已从 48 → 49，仅新增 1 个诊断查询）
- 不切换 HTTP/SSE/WebSocket 传输协议
- 不实现 UE 侧真正多客户端并发会话池（已固定为单客户端独占模型）
- 不做远程部署、TLS、LAN 暴露
- 多 Graph 类型 / 更新既有 Blueprint / 更多节点白名单（暂不扩展）
- 为所有 AI 客户端各写一套 skill（已拒绝，改为 P2 Resources 方案）
- 仓库内完整计划与外部主计划双向维护（单向同步，外部为主）

### 12A 关键变更

- **单客户端模型**：已文档化，第二连接被显式拒绝（`CLIENT_ALREADY_CONNECTED`），不再依赖隐式 accept 时机
- **运行时诊断**：新增 `get_bridge_runtime_status`（49th Handler），暴露 server_status / last_error / client_connected / transport_mode 等字段
- **Python 错误分类**：`UEBridgeError` 携带分类码（CONNECT_TIMEOUT / READ_TIMEOUT / PEER_CLOSED / CLIENT_ALREADY_CONNECTED / RESPONSE_MISMATCH）
- **启动失败可见**：`SetLastError()` 记录 Bind/Listen 错误，模块 startup warning 结构化输出

---

## 架构摘要

```
MCP Client (Claude/Codex)
    ↕ stdio (JSON-RPC)
Python MCP Server (server.py)
    ↕ TCP/JSON Lines (127.0.0.1:9876)
UE Editor Plugin (UnrealEditorMCPBridge)
    ↕ UE API
Unreal Engine 5.3 Editor
```

- UE 插件负责编辑器生命周期、请求分发、Game Thread 调度
- Python Server 负责 MCP 协议、tool 注册、客户端集成
- 仅本地回环，**单客户端独占** TCP 连接（第二连接被拒绝并返回 CLIENT_ALREADY_CONNECTED）

---

## 能力清单（49 Handler）

| 类别 | 数量 | 工具 |
|------|------|------|
| 编辑器状态 | 7 | ping, get_bridge_runtime_status, editor_info, project_info, world_state, get_mcp_config, dirty_packages |
| 资产浏览 | 6 | list_assets, asset_info, list_blueprints, list_materials, list_widgets, widget_info |
| Actor 操作 | 6 | selected_actors, level_actors, get_actor_property, get_component_property, spawn, delete |
| 编辑操作 | 5 | set_transform, set_actor_property, set_component_property, save_level, viewport_screenshot |
| 资产编辑 | 10 | create_blueprint, add_variable, add_function, create_widget_bp, widget_add/remove/set, material_set_scalar/vector/texture |
| Blueprint 图编辑 | 8 | create_actor_class, event_graph_info, add_event_node, add_call_function_node, connect_pins, compile_save, apply_spec, export_spec |
| 事务控制 | 4 | begin/end/undo/redo |
| 10C 自举 | - | bootstrap config + offline degradation |
| Python | 1 | execute_python_snippet (Dangerous) |
| 蓝图/材质信息 | 2 | get_blueprint_info, get_material_info |

---

## 关键路径文件

```
UnrealEditorMCP/                          ← Git 仓库根
├── README.md
├── .gitignore / .gitattributes / .editorconfig
├── AGENTS.md
│
├── UnrealEditorMCP/                      ← UE 项目根
│   ├── UnrealEditorMCP.uproject
│   ├── .ai/
│   │   ├── plan/plan.md                  # 项目内计划
│   │   ├── plan/plan_log.md              # 变更日志
│   │   ├── plan/plan_sync.md             # 本文件（协作镜像）
│   │   ├── dev/dev_last.md               # 最近操作
│   │   └── log/                          # 操作日志
│   │
│   ├── Plugins/UnrealEditorMCPBridge/
│   │   ├── Config/DefaultUnrealEditorMCPBridge.ini
│   │   └── Source/UnrealEditorMCPBridge/
│   │       ├── Public/
│   │       │   ├── MCPBridgeServer.h
│   │       │   ├── MCPBridgeHandler.h
│   │       │   ├── MCPBridgeDispatcher.h
│   │       │   ├── MCPBridgeHelpers.h
│   │       │   ├── MCPBlueprintGraphHelpers.h
│   │       │   ├── MCPBlueprintSpecTypes.h
│   │       │   └── Handlers/             # 13 个 Handler 头文件
│   │       └── Private/
│   │           ├── MCPBridgeServer.cpp    # Server + Handler 注册
│   │           ├── MCPBlueprintGraphHelpers.cpp
│   │           └── Handlers/             # 13 个 Handler 实现
│   │
│   └── Tools/MCPBridgeServer/
│       ├── pyproject.toml
│       └── src/mcp_bridge_server/
│           ├── server.py                 # MCP 入口 + bootstrap fallback
│           ├── bridge_client.py          # TCP 客户端
│           └── tool_schemas.py           # 48 tool schema 注册表
```

---

## 核心决策记录

1. 架构：UE 插件宿主 + 外部 Python MCP Server + localhost TCP JSON Lines
2. 权限：三级分类（Read/Write/Dangerous）+ 可选 token，默认开发模式开放
3. Blueprint：11A 受控命令面 → 11B 声明式 spec 重建，不做通用图编辑器
4. 安全：仅 127.0.0.1，不做远程暴露
5. 传输层：保持当前 TCP，不预设切换 HTTP/SSE（阶段 12A 已收口：单客户端独占 + 运行时诊断 + 错误分类）
6. 知识层：拒绝客户端专属 skill，改为 P2 MCP Resources 方案
7. 计划：外部 Kilo plan 为主，repo 内 plan_sync.md 为单向协作镜像
