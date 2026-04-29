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

## Handler 清单（46 个）

```
Read (20): ping / editor_info / project_info / world_state / list_assets
           get_asset_info / get_mcp_config / selected_actors / level_actors
           get_actor_property / get_component_property / list_blueprints
           get_bp_info / blueprint_get_event_graph_info / list_materials
           get_material_info / list_widgets / get_widget_info
           viewport_screenshot / get_dirty_packages

Write (25): spawn_actor / set_transform / set_actor_property / save_level
            delete_actor / set_component_property / begin/end/undo/redo
            create_blueprint / blueprint_add_variable / blueprint_add_function
            blueprint_create_actor_class / blueprint_add_event_node
            blueprint_add_call_function_node / blueprint_connect_pins
            blueprint_compile_save / create_widget_bp
            widget_add_child / widget_remove_child / widget_set_property
            material_set_scalar/vector/texture_param

Dangerous (1): execute_python_snippet
```

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
