# Unreal Editor MCP Bridge

UE 5.3 编辑器插件 + Python MCP Server，通过本地 TCP JSON Lines 桥接，使 MCP 客户端（如 Claude / Codex）能操控 Unreal Engine 编辑器。

## 架构

```
MCP Client (Claude/Codex)
    ↕ stdio (JSON-RPC)
Python MCP Server (server.py)
    ↕ TCP/JSON Lines (127.0.0.1:9876)
UE Editor Plugin (UnrealEditorMCPBridge)
    ↕ UE API
Unreal Engine 5.3 Editor
```

**传输层可替换**，当前使用本地 TCP + 外部 Python Server，后续可评估 HTTP/SSE。

## 能力清单（58 Handler）

### 编辑器状态（7）
| 工具 | 说明 |
|------|------|
| `ue_ping` | 连通性测试 |
| `ue_get_bridge_runtime_status` | 传输层运行时状态（server_status / client_connected / last_error） |
| `ue_get_editor_info` | 引擎版本、编辑器标识 |
| `ue_get_project_info` | 项目名称、磁盘路径 |
| `ue_get_world_state` | 世界名、Actor 数、World Partition 状态 |
| `ue_get_mcp_config` | 返回全部 action 注册表、模式、版本 |
| `ue_get_dirty_packages` | 列出所有未保存的脏包 |

### 资产浏览（6）
| 工具 | 说明 |
|------|------|
| `ue_list_assets` | AssetRegistry 资产列表（路径/递归/类过滤） |
| `ue_get_asset_info` | 单个资产详情 |
| `ue_list_blueprints` | 列出 Blueprint 资产 |
| `ue_list_materials` | 列出 Material/MaterialInstance |
| `ue_list_widgets` | 列出 Widget Blueprint |
| `ue_get_widget_info` | Widget 树结构（root / parent / slot_class / is_variable） |

### Actor 操作（6）
| 工具 | 说明 |
|------|------|
| `ue_get_selected_actors` | 选中 Actor 列表（含 Transform） |
| `ue_list_level_actors` | 关卡全部 Actor（含 Transform） |
| `ue_level_get_actor_property` | 反射读取 Actor 属性 |
| `ue_level_get_component_property` | 反射读取组件属性 |
| `ue_spawn_actor` | 生成 Actor（类名+Transform） |
| `ue_actor_delete` | 删除 Actor |

### 编辑操作（5）
| 工具 | 说明 |
|------|------|
| `ue_level_set_actor_transform` | 设置 Actor 位置/旋转/缩放 |
| `ue_actor_set_property` | 反射写入 Actor 属性 + read-back |
| `ue_component_set_property` | 反射写入组件属性 + read-back |
| `ue_save_current_level` | 保存关卡 |
| `ue_viewport_screenshot` | 视口截图→PNG |

### 资产编辑（11）
| 工具 | 说明 |
|------|------|
| `ue_create_blueprint` | 创建 Blueprint（指定父类/路径，含冲突检测） |
| `ue_blueprint_add_variable` | 添加变量（bool/int/float/string/Vector/Rotator/Color） |
| `ue_blueprint_add_function` | 添加函数图（含重名检测） |
| `ue_blueprint_add_component` | 向 BP 的 SCS 添加组件（任意 UActorComponent 子类） |
| `ue_create_widget_blueprint` | 创建 Widget Blueprint（可选 root_widget_class） |
| `ue_widget_add_child` | 向 Widget 树添加控件（可选 index 插入位置） |
| `ue_widget_remove_child` | 从 Widget 树移除控件 |
| `ue_widget_set_property` | 设置 Widget 属性 + read-back |
| `ue_material_set_scalar_param` | 设置 MaterialInstance 标量参数 |
| `ue_material_set_vector_param` | 设置 MaterialInstance 向量/颜色参数 |
| `ue_material_set_texture_param` | 设置 MaterialInstance 纹理参数 |

### Blueprint 图编辑（8）
| 工具 | 说明 |
|------|------|
| `ue_blueprint_create_actor_class` | 创建 Actor Blueprint 类 |
| `ue_blueprint_get_event_graph_info` | 获取 EventGraph 结构信息 |
| `ue_blueprint_add_event_node` | 添加/复用事件节点 |
| `ue_blueprint_add_call_function_node` | 添加 CallFunction 节点 |
| `ue_blueprint_connect_pins` | 连接引脚 |
| `ue_blueprint_compile_save` | 编译 Blueprint（可选保存） |
| `ue_blueprint_apply_spec` | 从声明式 spec JSON 创建 BP 图 |
| `ue_blueprint_export_spec` | 导出已有 BP 的 EventGraph 为 spec |

### Widget 深化编辑（10）
| 工具 | 说明 |
|------|------|
| `ue_widget_get_property_schema` | 查询控件可编辑属性清单（name/type/editable） |
| `ue_widget_get_slot_schema` | 查询控件 slot 类及可编辑属性 |
| `ue_widget_find` | 按 name/class 搜索 Widget 树节点 |
| `ue_widget_set_root` | 设定/替换 root widget |
| `ue_widget_reparent` | 移动节点到新 parent（含循环检测） |
| `ue_widget_reorder_child` | 调整 sibling 顺序 |
| `ue_widget_rename` | 重命名节点（含冲突检测） |
| `ue_widget_set_slot_property` | 设置 slot 布局属性（padding/alignment/anchors） |
| `ue_widget_duplicate` | 复制节点/子树 |
| `ue_widget_wrap_with_panel` | 用容器面板包裹现有控件 |

### 事务控制（4）
| 工具 | 说明 |
|------|------|
| `ue_begin_transaction` | 开始事务（重入拒绝） |
| `ue_end_transaction` | 结束事务（空拒绝） |
| `ue_undo` | 撤销（自动结束活跃事务） |
| `ue_redo` | 重做（自动结束活跃事务） |

### Python / 信息（3）
| 工具 | 说明 |
|------|------|
| `ue_execute_python_snippet` | 执行 UE Python 代码（Dangerous） |
| `ue_get_blueprint_info` | BP 详情：父类/变量/函数/接口 |
| `ue_get_material_info` | 材质详情：blend_mode/参数列表 |

### MCP Resources（6）
| Resource URI | 类型 | 说明 |
|------|------|------|
| `ue://resources/overview` | Static | 项目架构、Handler 分类、单客户端模型 |
| `ue://resources/error-model` | Static | UEBridgeError 分类、所有错误码、客户端指导 |
| `ue://resources/workflows` | Static | 常用调用链（BP 创建/编辑/Spec/Widget/事务/材质） |
| `ue://resources/blueprint-spec` | Static | 11A/11B spec 格式、边界、round-trip、常见问题 |
| `ue://runtime/config` | Live | 当前 action registry / mode / token_enabled |
| `ue://runtime/status` | Live | 当前 server_status / client_connected / last_error |

## 目录

```
UnrealEditorMCP/                    ← Git 仓库根目录
├── UnrealEditorMCP.uproject
├── README.md
├── .gitignore
├── UnrealEditorMCP/              ← UE 项目根目录
│   ├── Config/
│   ├── Content/
│   ├── Source/
│   ├── Plugins/UnrealEditorMCPBridge/   # UE 编辑器插件
│   │   ├── Config/                      # ini 配置（Token/Port）
│   │   ├── Source/UnrealEditorMCPBridge/
│   │   │   ├── Public/                  # Handler 接口、调度器、Helpers
│   │   │   │   └── Handlers/            # 11 个领域文件（58 Handler）
│   │   │   └── Private/                 # 实现
│   │   └── Resources/
│   ├── Tools/MCPBridgeServer/           # Python MCP Server
│   │   ├── src/mcp_bridge_server/
│   │   │   ├── server.py                # MCP 入口
│   │   │   ├── bridge_client.py         # TCP 客户端
│   │   │   ├── resources.py             # MCP Resources 知识层（4 static + 2 live）
│   │   │   └── tool_schemas.py          # 58 tool schema 注册表
│   │   └── tests/
│   │       ├── test_stage12a.ps1          # 传输层稳态化验收
│   │       ├── test_stage14_resources.ps1 # 资源层验收（双模）
│   │       ├── test_resources_mcp.py      # MCP 协议层精确测试
│   │       └── test_stage16_widget_deep.ps1 # Widget 深化验收
│   └── .ai/                             # 项目治理（plan / log / dev）
└── Config/                              # 项目工程配置
```

## 运行前提

- **UE 5.3**（引擎路径通过 Epic Launcher 配置）
- Python 3.10+，安装依赖：`pip install mcp`
- 工程需启用插件：`PythonScriptPlugin`、`EditorScriptingUtilities`（`.uproject` 已配置）

## 启动

### 1. 启动 UE Editor
```
UnrealEditor.exe "UnrealEditorMCP.uproject"
```
插件加载后，确认日志输出：`MCP Bridge listening on 127.0.0.1:9876`

### 2. 启动 Python MCP Server
```bash
cd Tools/MCPBridgeServer
pip install -e .
python -m mcp_bridge_server
```

### 3. 注册到 MCP 客户端
在 Claude Desktop / Codex 的 MCP 配置中添加：
```json
{
  "mcpServers": {
    "ue-bridge": {
      "command": "python",
      "args": ["-m", "mcp_bridge_server"],
      "cwd": "Tools/MCPBridgeServer"
    }
  }
}
```

## 配置

编辑 `Plugins/UnrealEditorMCPBridge/Config/DefaultUnrealEditorMCPBridge.ini`：

```ini
[UnrealEditorMCPBridge]
; 共享密钥，留空 = 开发模式（全部开放），填写 = 受限模式（Write/Dangerous 需 token）
Token=
; 监听端口
Port=9876
```

**模式说明：**
- **开发模式**（Token 留空）：所有操作不受限制
- **受限模式**（Token 非空）：Write 类操作需携带 `_token` 匹配；Python 端传 `UEMCPServer(token="...")` 自动注入

## 常见问题

| 现象 | 解决 |
|------|------|
| 端口被占用 | 关闭旧 Editor 进程或等待 2 分钟 TIME_WAIT 释放 |
| UE Editor 未连接 | 确认插件编译并加载，检查日志 `MCP Bridge listening` |
| Python 执行失败 `PYTHON_UNAVAILABLE` | 确认 `.uproject` 已启用 `PythonScriptPlugin` |
| 创建资产 `ASSET_CONFLICT` | 目标路径已有同名资产 |
| 双重 `begin_transaction` | 先调用 `end_transaction` 再重新开始 |
| 断连后事务悬挂 | 插件自动结束活跃事务并丢弃残留请求 |

## 权限模型

```
Read (26)：ping / status / *_info / list_* / get_* / viewport_screenshot / dirty_packages / widget_*_schema / widget_find
Write (31)：spawn / set_* / delete / save / create_* / transaction / material_set_* / blueprint_add_* / widget_*
Dangerous (1)：execute_python_snippet
```

审计日志：所有 Write/Dangerous 操作自动输出 `AUDIT:` 日志。

## 当前限制

- 仅本地回环（127.0.0.1），不支持远程访问
- 单客户端独占 TCP 连接模式（第二连接被拒绝并返回 CLIENT_ALREADY_CONNECTED 错误）
- 运行时诊断：支持 `ue_get_bridge_runtime_status` + `ue_get_mcp_config` + `ue://runtime/*` Resources
- Blueprint：支持变量/函数添加 + 组件添加 + 节点级图编辑 + 声明式 spec 重建
- Widget：完整 UMG Designer 闭环（创建/查询/schema/树编辑/slot/属性/wrap/duplicate）
- Python 协议桥：线程安全（UEBridgeClient.send 实例级 Lock 串行化）
- 不包含 Sequencer / Niagara / Animation 操作

## License

MIT
