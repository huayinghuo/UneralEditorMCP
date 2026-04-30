"""MCP Tool Schema 注册表 —— 48 个 tool 的 action→（名称/描述/参数schema）映射

C++ Handler 通过 get_mcp_config 暴露 action 列表，Python 端从此注册表取 schema，
两者取交集生成最终 tool 列表，避免手工双写漂移。

Bootstrap Config：UE 不在线时，Python 侧自举暴露最小配置，list_tools()
与 call_tool("ue_get_mcp_config") 断连退化均来自此单一来源。
"""

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
}

TOOL_SCHEMAS = {
    "ping": {
        "name": "ue_ping",
        "description": "Test connectivity with the UE Editor bridge",
        "inputSchema": {
            "type": "object", "properties": {}, "required": []
        },
    },
    "get_bridge_runtime_status": {
        "name": "ue_get_bridge_runtime_status",
        "description": "Get bridge transport layer runtime status (server state, connection info, last error)",
        "inputSchema": {
            "type": "object", "properties": {}, "required": []
        },
    },
    "get_editor_info": {
        "name": "ue_get_editor_info",
        "description": "Get information about the running Unreal Editor instance",
        "inputSchema": {
            "type": "object", "properties": {}, "required": []
        },
    },
    "get_project_info": {
        "name": "ue_get_project_info",
        "description": "Get information about the current Unreal project",
        "inputSchema": {
            "type": "object", "properties": {}, "required": []
        },
    },
    "get_world_state": {
        "name": "ue_get_world_state",
        "description": "Get overview of the current editor world (name, actor count, selection count)",
        "inputSchema": {
            "type": "object", "properties": {}, "required": []
        },
    },
    "list_assets": {
        "name": "ue_list_assets",
        "description": "List assets in the project by path using the Asset Registry",
        "inputSchema": {
            "type": "object",
            "properties": {
                "path": {"type": "string", "description": "Content path (default /Game)"},
                "recursive": {"type": "boolean", "description": "Search subdirectories (default true)"},
                "class_name": {"type": "string", "description": "Optional class filter (e.g. Texture2D)"},
            },
            "required": [],
        },
    },
    "get_asset_info": {
        "name": "ue_get_asset_info",
        "description": "Get detailed information about a single asset by path",
        "inputSchema": {
            "type": "object",
            "properties": {
                "asset_path": {"type": "string", "description": "Full asset path"},
            },
            "required": ["asset_path"],
        },
    },
    "get_mcp_config": {
        "name": "ue_get_mcp_config",
        "description": "Get MCP bridge configuration and action registry",
        "inputSchema": {
            "type": "object", "properties": {}, "required": []
        },
    },
    # ---- Actor ----
    "get_selected_actors": {
        "name": "ue_get_selected_actors",
        "description": "Get the currently selected actors in the Unreal Editor",
        "inputSchema": {
            "type": "object", "properties": {}, "required": []
        },
    },
    "list_level_actors": {
        "name": "ue_list_level_actors",
        "description": "List all actors in the current level",
        "inputSchema": {
            "type": "object", "properties": {}, "required": []
        },
    },
    "level_get_actor_property": {
        "name": "ue_level_get_actor_property",
        "description": "Read a property value from an actor by name using UE reflection",
        "inputSchema": {
            "type": "object",
            "properties": {
                "actor_name": {"type": "string"},
                "property_name": {"type": "string"},
            },
            "required": ["actor_name", "property_name"],
        },
    },
    "level_get_component_property": {
        "name": "ue_level_get_component_property",
        "description": "Read a property value from a component on an actor by name using UE reflection",
        "inputSchema": {
            "type": "object",
            "properties": {
                "actor_name": {"type": "string"},
                "component_name": {"type": "string"},
                "property_name": {"type": "string"},
            },
            "required": ["actor_name", "component_name", "property_name"],
        },
    },
    # ---- Python ----
    "execute_python_snippet": {
        "name": "ue_execute_python_snippet",
        "description": "Execute a Python snippet in the Unreal Editor's Python environment",
        "inputSchema": {
            "type": "object",
            "properties": {
                "code": {"type": "string", "description": "Python code to execute"},
            },
            "required": ["code"],
        },
    },
    # ---- Write ----
    "spawn_actor": {
        "name": "ue_spawn_actor",
        "description": "Spawn a new actor in the current level by class name",
        "inputSchema": {
            "type": "object",
            "properties": {
                "class_name": {"type": "string"},
                "location": {"type": "object", "description": "{x, y, z}"},
                "rotation": {"type": "object", "description": "{pitch, yaw, roll}"},
                "scale": {"type": "object", "description": "{x, y, z}"},
            },
            "required": ["class_name"],
        },
    },
    "level_set_actor_transform": {
        "name": "ue_level_set_actor_transform",
        "description": "Set the transform of an actor by name",
        "inputSchema": {
            "type": "object",
            "properties": {
                "actor_name": {"type": "string"},
                "location": {"type": "object"},
                "rotation": {"type": "object"},
                "scale": {"type": "object"},
            },
            "required": ["actor_name"],
        },
    },
    "actor_set_property": {
        "name": "ue_actor_set_property",
        "description": "Set a property value on an actor by name using UE reflection",
        "inputSchema": {
            "type": "object",
            "properties": {
                "actor_name": {"type": "string"},
                "property_name": {"type": "string"},
                "value": {"type": "string"},
            },
            "required": ["actor_name", "property_name", "value"],
        },
    },
    "save_current_level": {
        "name": "ue_save_current_level",
        "description": "Save the currently open level to disk",
        "inputSchema": {
            "type": "object", "properties": {}, "required": []
        },
    },
    "actor_delete": {
        "name": "ue_actor_delete",
        "description": "Delete an actor from the level by name",
        "inputSchema": {
            "type": "object",
            "properties": {
                "actor_name": {"type": "string"},
            },
            "required": ["actor_name"],
        },
    },
    "component_set_property": {
        "name": "ue_component_set_property",
        "description": "Set a property value on a component by actor and component name",
        "inputSchema": {
            "type": "object",
            "properties": {
                "actor_name": {"type": "string"},
                "component_name": {"type": "string"},
                "property_name": {"type": "string"},
                "value": {"type": "string"},
            },
            "required": ["actor_name", "component_name", "property_name", "value"],
        },
    },
    # ---- Transaction ----
    "begin_transaction": {
        "name": "ue_begin_transaction",
        "description": "Begin an undo-able editor transaction",
        "inputSchema": {
            "type": "object",
            "properties": {
                "description": {"type": "string"},
            },
            "required": [],
        },
    },
    "end_transaction": {
        "name": "ue_end_transaction",
        "description": "End the current editor transaction",
        "inputSchema": {
            "type": "object", "properties": {}, "required": []
        },
    },
    "undo": {
        "name": "ue_undo",
        "description": "Undo the last transaction",
        "inputSchema": {
            "type": "object", "properties": {}, "required": []
        },
    },
    "redo": {
        "name": "ue_redo",
        "description": "Redo the last undone transaction",
        "inputSchema": {
            "type": "object", "properties": {}, "required": []
        },
    },
    # ---- Blueprint ----
    "list_blueprints": {
        "name": "ue_list_blueprints",
        "description": "List all Blueprint assets in the project",
        "inputSchema": {
            "type": "object",
            "properties": {
                "path": {"type": "string"},
            },
            "required": [],
        },
    },
    "get_blueprint_info": {
        "name": "ue_get_blueprint_info",
        "description": "Get detailed information about a Blueprint (parent, variables, functions, interfaces)",
        "inputSchema": {
            "type": "object",
            "properties": {
                "asset_path": {"type": "string"},
            },
            "required": ["asset_path"],
        },
    },
    "create_blueprint": {
        "name": "ue_create_blueprint",
        "description": "Create a new Blueprint asset with a given parent class",
        "inputSchema": {
            "type": "object",
            "properties": {
                "name": {"type": "string"},
                "parent_class": {"type": "string"},
                "path": {"type": "string"},
            },
            "required": ["name"],
        },
    },
    "blueprint_add_variable": {
        "name": "ue_blueprint_add_variable",
        "description": "Add a member variable to an existing Blueprint",
        "inputSchema": {
            "type": "object",
            "properties": {
                "asset_path": {"type": "string"},
                "var_name": {"type": "string"},
                "var_type": {"type": "string", "description": "bool/int/float/string/Vector/Rotator/Color"},
                "default_value": {"type": "string"},
            },
            "required": ["asset_path", "var_name"],
        },
    },
    "blueprint_add_function": {
        "name": "ue_blueprint_add_function",
        "description": "Add a new function graph to an existing Blueprint",
        "inputSchema": {
            "type": "object",
            "properties": {
                "asset_path": {"type": "string"},
                "func_name": {"type": "string"},
            },
            "required": ["asset_path", "func_name"],
        },
    },
    "blueprint_add_component": {
        "name": "ue_blueprint_add_component",
        "description": "Add a component to a Blueprint's SimpleConstructionScript (e.g. StaticMeshComponent, SceneComponent)",
        "inputSchema": {
            "type": "object",
            "properties": {
                "asset_path": {"type": "string", "description": "Blueprint asset path"},
                "component_class": {"type": "string", "description": "Component class name, e.g. StaticMeshComponent, SceneComponent, BoxComponent"},
                "component_name": {"type": "string", "description": "Optional component name (default: class name)"},
            },
            "required": ["asset_path", "component_class"],
        },
    },
    # ---- Blueprint Graph Editing (Stage 11A) ----
    "blueprint_create_actor_class": {
        "name": "ue_blueprint_create_actor_class",
        "description": "Create a new Actor Blueprint class (parent must be Actor-derived)",
        "inputSchema": {
            "type": "object",
            "properties": {
                "name": {"type": "string", "description": "Blueprint name"},
                "parent_class": {"type": "string", "description": "Parent class name (default: Actor)"},
                "path": {"type": "string", "description": "Content path (default: /Game/Blueprints)"},
            },
            "required": ["name"],
        },
    },
    "blueprint_get_event_graph_info": {
        "name": "ue_blueprint_get_event_graph_info",
        "description": "Get EventGraph info: existence, event node count, BeginPlay node GUID",
        "inputSchema": {
            "type": "object",
            "properties": {
                "asset_path": {"type": "string", "description": "Blueprint asset path"},
            },
            "required": ["asset_path"],
        },
    },
    "blueprint_add_event_node": {
        "name": "ue_blueprint_add_event_node",
        "description": "Add or reuse an event node (ReceiveBeginPlay) in the EventGraph",
        "inputSchema": {
            "type": "object",
            "properties": {
                "asset_path": {"type": "string", "description": "Blueprint asset path"},
                "event_name": {"type": "string", "description": "Event member name (default: ReceiveBeginPlay)"},
            },
            "required": ["asset_path"],
        },
    },
    "blueprint_add_call_function_node": {
        "name": "ue_blueprint_add_call_function_node",
        "description": "Create a CallFunction node for a BlueprintCallable function in the EventGraph",
        "inputSchema": {
            "type": "object",
            "properties": {
                "asset_path": {"type": "string", "description": "Blueprint asset path"},
                "function_name": {"type": "string", "description": "BlueprintCallable function name to call"},
                "graph_name": {"type": "string", "description": "Target graph name (default: EventGraph)"},
            },
            "required": ["asset_path", "function_name"],
        },
    },
    "blueprint_connect_pins": {
        "name": "ue_blueprint_connect_pins",
        "description": "Connect two pins by node GUID and pin name",
        "inputSchema": {
            "type": "object",
            "properties": {
                "asset_path": {"type": "string", "description": "Blueprint asset path"},
                "source_node_guid": {"type": "string", "description": "Source node GUID"},
                "source_pin_name": {"type": "string", "description": "Source output pin name"},
                "target_node_guid": {"type": "string", "description": "Target node GUID"},
                "target_pin_name": {"type": "string", "description": "Target input pin name"},
                "graph_name": {"type": "string", "description": "Target graph name (default: EventGraph)"},
            },
            "required": ["asset_path", "source_node_guid", "source_pin_name", "target_node_guid", "target_pin_name"],
        },
    },
    "blueprint_compile_save": {
        "name": "ue_blueprint_compile_save",
        "description": "Compile a Blueprint, optionally save it to disk",
        "inputSchema": {
            "type": "object",
            "properties": {
                "asset_path": {"type": "string", "description": "Blueprint asset path"},
                "save": {"type": "boolean", "description": "Also save to disk (default: false)"},
            },
            "required": ["asset_path"],
        },
    },
    # ---- Blueprint Spec (Stage 11B) ----
    "blueprint_apply_spec": {
        "name": "ue_blueprint_apply_spec",
        "description": "Create a Blueprint from a declarative spec (blueprint, nodes, edges). First pass: Actor Blueprint + EventGraph only.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "spec": {
                    "type": "object",
                    "description": "Blueprint spec: {blueprint: {name, parent_class}, nodes: [{id, type, event_name|function_name, params}], edges: [{from_node, from_pin, to_node, to_pin}]}"
                },
            },
            "required": ["spec"],
        },
    },
    "blueprint_export_spec": {
        "name": "ue_blueprint_export_spec",
        "description": "Export an existing Blueprint's EventGraph as a declarative spec (nodes, pins, edges)",
        "inputSchema": {
            "type": "object",
            "properties": {
                "asset_path": {"type": "string", "description": "Blueprint asset path to export"},
            },
            "required": ["asset_path"],
        },
    },
    # ---- Material ----
    "list_materials": {
        "name": "ue_list_materials",
        "description": "List Material and Material Instance assets in the project",
        "inputSchema": {
            "type": "object",
            "properties": {
                "path": {"type": "string"},
            },
            "required": [],
        },
    },
    "get_material_info": {
        "name": "ue_get_material_info",
        "description": "Get detailed info about a Material or Material Instance (parameters, blend mode, etc)",
        "inputSchema": {
            "type": "object",
            "properties": {
                "asset_path": {"type": "string"},
            },
            "required": ["asset_path"],
        },
    },
    "material_set_scalar_param": {
        "name": "ue_material_set_scalar_param",
        "description": "Set a scalar parameter value on a Material Instance",
        "inputSchema": {
            "type": "object",
            "properties": {
                "asset_path": {"type": "string"},
                "param_name": {"type": "string"},
                "value": {"type": "number"},
            },
            "required": ["asset_path", "param_name", "value"],
        },
    },
    "material_set_vector_param": {
        "name": "ue_material_set_vector_param",
        "description": "Set a vector/color parameter on a Material Instance",
        "inputSchema": {
            "type": "object",
            "properties": {
                "asset_path": {"type": "string"},
                "param_name": {"type": "string"},
                "r": {"type": "number"}, "g": {"type": "number"},
                "b": {"type": "number"}, "a": {"type": "number"},
            },
            "required": ["asset_path", "param_name"],
        },
    },
    "material_set_texture_param": {
        "name": "ue_material_set_texture_param",
        "description": "Set a texture parameter on a Material Instance",
        "inputSchema": {
            "type": "object",
            "properties": {
                "asset_path": {"type": "string"},
                "param_name": {"type": "string"},
                "texture_path": {"type": "string"},
            },
            "required": ["asset_path", "param_name"],
        },
    },
    # ---- Viewport ----
    "viewport_screenshot": {
        "name": "ue_viewport_screenshot",
        "description": "Capture a screenshot of the current editor viewport and save to file",
        "inputSchema": {
            "type": "object",
            "properties": {
                "filename": {"type": "string"},
            },
            "required": [],
        },
    },
    # ---- Widget ----
    "list_widgets": {
        "name": "ue_list_widgets",
        "description": "List all Widget Blueprint assets in the project",
        "inputSchema": {
            "type": "object",
            "properties": {"path": {"type": "string"}},
            "required": [],
        },
    },
    "get_widget_info": {
        "name": "ue_get_widget_info",
        "description": "Get detailed info about a Widget Blueprint including its widget tree",
        "inputSchema": {
            "type": "object",
            "properties": {"asset_path": {"type": "string"}},
            "required": ["asset_path"],
        },
    },
    "create_widget_blueprint": {
        "name": "ue_create_widget_blueprint",
        "description": "Create a new Widget Blueprint asset",
        "inputSchema": {
            "type": "object",
            "properties": {
                "name": {"type": "string"},
                "path": {"type": "string"},
            },
            "required": ["name"],
        },
    },
    # ---- Dirty ----
    "get_dirty_packages": {
        "name": "ue_get_dirty_packages",
        "description": "List all modified (dirty) packages that need saving",
        "inputSchema": {
            "type": "object", "properties": {}, "required": []
        },
    },
    # ---- Widget Editing (Phase 11) ----
    "widget_add_child": {
        "name": "ue_widget_add_child",
        "description": "Add a child widget (TextBlock/Button/Image etc) to a Widget Blueprint tree",
        "inputSchema": {
            "type": "object",
            "properties": {
                "asset_path": {"type": "string", "description": "Widget Blueprint asset path"},
                "widget_class": {"type": "string", "description": "Widget class name (e.g. TextBlock, Button, Image)"},
                "parent_widget": {"type": "string", "description": "Optional parent panel name"},
                "widget_name": {"type": "string", "description": "Optional name for the new widget"},
            },
            "required": ["asset_path", "widget_class"],
        },
    },
    "widget_remove_child": {
        "name": "ue_widget_remove_child",
        "description": "Remove a widget from a Widget Blueprint tree by name",
        "inputSchema": {
            "type": "object",
            "properties": {
                "asset_path": {"type": "string"},
                "widget_name": {"type": "string"},
            },
            "required": ["asset_path", "widget_name"],
        },
    },
    "widget_set_property": {
        "name": "ue_widget_set_property",
        "description": "Set a property value on a widget in a Widget Blueprint",
        "inputSchema": {
            "type": "object",
            "properties": {
                "asset_path": {"type": "string"},
                "widget_name": {"type": "string"},
                "property_name": {"type": "string"},
                "value": {"type": "string"},
            },
            "required": ["asset_path", "widget_name", "property_name", "value"],
        },
    },
    # ── Stage 16 Widget Deepening ──
    "widget_get_property_schema": {
        "name": "ue_widget_get_property_schema",
        "description": "Get editable property schema for a widget (name, type, editable, read_only per property)",
        "inputSchema": {
            "type": "object",
            "properties": {
                "asset_path": {"type": "string"},
                "widget_name": {"type": "string"},
            },
            "required": ["asset_path", "widget_name"],
        },
    },
    "widget_get_slot_schema": {
        "name": "ue_widget_get_slot_schema",
        "description": "Get slot property schema for a widget's current slot (slot_class, editable properties)",
        "inputSchema": {
            "type": "object",
            "properties": {
                "asset_path": {"type": "string"},
                "widget_name": {"type": "string"},
            },
            "required": ["asset_path", "widget_name"],
        },
    },
    "widget_find": {
        "name": "ue_widget_find",
        "description": "Find widgets in a Widget Blueprint by name/class substring query",
        "inputSchema": {
            "type": "object",
            "properties": {
                "asset_path": {"type": "string"},
                "query": {"type": "string", "description": "Substring to match against widget name or class (empty = all)"},
            },
            "required": ["asset_path"],
        },
    },
    # ── Stage 16 Structure Editing ──
    "widget_set_root": {
        "name": "ue_widget_set_root",
        "description": "Set or replace the root widget of a Widget Blueprint",
        "inputSchema": {
            "type": "object",
            "properties": {
                "asset_path": {"type": "string"},
                "widget_class": {"type": "string", "description": "Widget class name, e.g. CanvasPanel, SizeBox"},
            },
            "required": ["asset_path", "widget_class"],
        },
    },
    "widget_reparent": {
        "name": "ue_widget_reparent",
        "description": "Move a widget to a new parent panel (with circular reference detection)",
        "inputSchema": {
            "type": "object",
            "properties": {
                "asset_path": {"type": "string"},
                "widget_name": {"type": "string"},
                "parent_widget": {"type": "string", "description": "Target panel widget name"},
            },
            "required": ["asset_path", "widget_name", "parent_widget"],
        },
    },
    "widget_reorder_child": {
        "name": "ue_widget_reorder_child",
        "description": "Change a widget's index within its parent panel",
        "inputSchema": {
            "type": "object",
            "properties": {
                "asset_path": {"type": "string"},
                "widget_name": {"type": "string"},
                "index": {"type": "integer", "description": "New zero-based index (default 0)"},
            },
            "required": ["asset_path", "widget_name"],
        },
    },
    "widget_rename": {
        "name": "ue_widget_rename",
        "description": "Rename a widget in the Widget Blueprint tree",
        "inputSchema": {
            "type": "object",
            "properties": {
                "asset_path": {"type": "string"},
                "widget_name": {"type": "string"},
                "new_name": {"type": "string"},
            },
            "required": ["asset_path", "widget_name", "new_name"],
        },
    },
    # ── Stage 16 Slot Editing ──
    "widget_set_slot_property": {
        "name": "ue_widget_set_slot_property",
        "description": "Set a slot layout property on a widget (e.g. padding, alignment, anchors)",
        "inputSchema": {
            "type": "object",
            "properties": {
                "asset_path": {"type": "string"},
                "widget_name": {"type": "string"},
                "property_name": {"type": "string"},
                "value": {"type": "string"},
            },
            "required": ["asset_path", "widget_name", "property_name", "value"],
        },
    },
    # ── Stage 16 Advanced Tree Operations ──
    "widget_duplicate": {
        "name": "ue_widget_duplicate",
        "description": "Duplicate a widget (or subtree) within the same parent",
        "inputSchema": {
            "type": "object",
            "properties": {
                "asset_path": {"type": "string"},
                "widget_name": {"type": "string"},
                "new_name": {"type": "string", "description": "Optional new name (default: original_Copy)"},
            },
            "required": ["asset_path", "widget_name"],
        },
    },
    "widget_wrap_with_panel": {
        "name": "ue_widget_wrap_with_panel",
        "description": "Wrap an existing widget inside a new container panel (Border, SizeBox, etc.)",
        "inputSchema": {
            "type": "object",
            "properties": {
                "asset_path": {"type": "string"},
                "widget_name": {"type": "string"},
                "panel_class": {"type": "string", "description": "Panel class name, e.g. Border, SizeBox, Overlay"},
                "wrapper_name": {"type": "string", "description": "Optional wrapper name"},
            },
            "required": ["asset_path", "widget_name", "panel_class"],
        },
    },
}
