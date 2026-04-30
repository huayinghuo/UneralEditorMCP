"""
MCP Resources — Static knowledge resources (offline-readable)
URI scheme: ue://resources/...

All content derived from implemented capabilities, not ideal designs.
"""
import json
import mcp.types as types

# ── Static Resource Definitions ──────────────────────────────────────────

RESOURCE_REGISTRY = {
    "ue://resources/overview": {
        "name": "UE Bridge Overview",
        "description": "Project architecture, 49 Handler categories, single-client model, bootstrap/live model",
        "mimeType": "text/markdown",
        "content": """# UnrealEditorMCP Bridge Overview

## Architecture

```
MCP Client (Claude/Codex)
    ↕ stdio (JSON-RPC)
Python MCP Server (server.py)
    ↕ TCP/JSON Lines (127.0.0.1:9876)
UE Editor Plugin (UnrealEditorMCPBridge)
    ↕ UE API
Unreal Engine 5.3 Editor
```

## 49 Handlers by Category

| Category | Count | Examples |
|----------|-------|----------|
| Editor State | 7 | ping, get_bridge_runtime_status, get_editor_info, get_project_info, get_world_state, get_mcp_config, get_dirty_packages |
| Asset Browsing | 6 | list_assets, get_asset_info, list_blueprints, list_materials, list_widgets, get_widget_info |
| Actor Operations | 6 | get_selected_actors, list_level_actors, get_actor_property, get_component_property, spawn_actor, actor_delete |
| Level Editing | 5 | level_set_actor_transform, actor_set_property, component_set_property, save_current_level, viewport_screenshot |
| Asset Editing | 10 | create_blueprint, blueprint_add_variable, blueprint_add_function, create_widget_blueprint, widget_add_child, widget_remove_child, widget_set_property, material_set_scalar_param, material_set_vector_param, material_set_texture_param |
| Blueprint Graph Editing | 8 | blueprint_create_actor_class, blueprint_get_event_graph_info, blueprint_add_event_node, blueprint_add_call_function_node, blueprint_connect_pins, blueprint_compile_save, blueprint_apply_spec, blueprint_export_spec |
| Transaction Control | 4 | begin_transaction, end_transaction, undo, redo |
| Blueprint/Material Info | 2 | get_blueprint_info, get_material_info |
| Python | 1 | execute_python_snippet (Dangerous) |

## Connection Model

- **Single-client exclusive**: Only one TCP client allowed at a time
- Second client is rejected with `CLIENT_ALREADY_CONNECTED` error
- Transport: TCP JSON Lines on `127.0.0.1:9876`
- Disconnect recovery: auto-clears queues, ends active transactions, resets to Listening

## Bootstrap / Live Model

- `ue_get_mcp_config` is the only tool available offline (returns bootstrap config with `source=python-bootstrap`)
- When UE is online, `source` switches to `ue-live` with full 49-action registry
- `list_tools()` dynamically generates tools from C++ action registry ∩ Python TOOL_SCHEMAS
""",
    },
    "ue://resources/error-model": {
        "name": "UE Bridge Error Model",
        "description": "UEBridgeError categories, common error codes, client guidance",
        "mimeType": "text/markdown",
        "content": """# UnrealEditorMCP Error Model

## Transport Errors (UEBridgeError.code)

| Code | Meaning | Typical Cause |
|------|---------|---------------|
| `CONNECT_TIMEOUT` | TCP connect timed out | UE Editor not running |
| `CONNECT_REFUSED` | TCP connection refused | Bridge plugin not loaded / port not listening |
| `CONNECT_FAILED` | Generic connect failure | Network or OS-level error |
| `READ_TIMEOUT` | Response read timed out | UE Editor busy or hung |
| `PEER_CLOSED` | UE closed the connection | UE Editor shut down |
| `CLIENT_ALREADY_CONNECTED` | Second client rejected | Another MCP client is already connected (single-client model) |
| `RESPONSE_MISMATCH` | Response ID doesn't match request | Protocol desync or concurrent requests on same socket |
| `CONNECTION_LOST` | Connection lost during send/recv | Network issue or UE process crash |

## Server-Level Errors (from UE C++ side)

| Code | Meaning |
|------|---------|
| `SOCKET_SUBSYSTEM_FAILED` | Platform socket subsystem unavailable |
| `CREATE_SOCKET_FAILED` | Listener socket creation failed |
| `BIND_FAILED` | Port binding failed (port already in use?) |
| `LISTEN_FAILED` | Socket listen() failed |
| `PARSE_ERROR` | Invalid JSON received |
| `MISSING_ACTION` | Request missing `action` field |
| `UNKNOWN_ACTION` | Action not registered in Dispatcher |

## Operational Error Codes (per-Handler)

| Code | Scope |
|------|-------|
| `MISSING_PARAM` | Any tool — required parameter missing |
| `TOKEN_REQUIRED` | Write/Dangerous tools — token not provided in restricted mode |
| `NO_WORLD` | Actor tools — no editor world available |
| `ACTOR_NOT_FOUND` / `COMPONENT_NOT_FOUND` | Actor/Component lookup failed |
| `BP_NOT_FOUND` / `WIDGET_NOT_FOUND` / `MATERIAL_NOT_FOUND` | Asset lookup failed |
| `CLASS_NOT_FOUND` | Parent class not found during create |
| `FUNCTION_NOT_FOUND` / `FUNCTION_NOT_CALLABLE` | Blueprint function lookup failed |
| `NODE_NOT_FOUND` / `PIN_NOT_FOUND` / `GRAPH_NOT_FOUND` | Blueprint graph node/pin/graph lookup failed |
| `SAVE_FAILED` / `CREATE_FAILED` / `COMPILE_FAILED` | Asset persistence/compilation failure |
| `ASSET_CONFLICT` / `NAME_CONFLICT` / `DUPLICATE_*` | Name/asset collision |
| `UNDO_FAILED` / `REDO_FAILED` | Transaction undo/redo failed |
| `TRANSACTION_ACTIVE` / `NO_ACTIVE_TRANSACTION` | Transaction state machine violation |

## Client Guidance

1. Always check `ok` field in response before reading `result`
2. On error, read `error.code` to determine recovery strategy:
   - Transport errors → check UE availability, reconnect
   - `CLIENT_ALREADY_CONNECTED` → wait and retry
   - `PARSE_ERROR` / `MISSING_ACTION` → fix request payload
   - `UNKNOWN_ACTION` → check tool availability via `list_tools()`
   - Handler errors → check parameter validity
3. Use `get_bridge_runtime_status` to diagnose server state before retrying
""",
    },
    "ue://resources/workflows": {
        "name": "UE Bridge Workflows",
        "description": "Common call chains: Blueprint creation/editing, Widget tree editing, transaction wrapping",
        "mimeType": "text/markdown",
        "content": """# UnrealEditorMCP Common Workflows

## Blueprint Creation (Simple)

```
ue_create_blueprint(name="MyActor", parent_class="Actor")
ue_blueprint_add_variable(asset_path="...", var_name="Health", var_type="float")
ue_blueprint_add_function(asset_path="...", func_name="OnTakeDamage")
ue_save_current_level()
```

## Blueprint Graph Editing (Stage 11A/11B)

```
# 1. Create Actor Blueprint
ue_blueprint_create_actor_class(name="MyBPA", parent_class="Actor")

# 2. Inspect EventGraph
ue_blueprint_get_event_graph_info(asset_path="/Game/Blueprints/MyBPA")

# 3. Add nodes
ue_blueprint_add_event_node(asset_path="...", event_name="ReceiveBeginPlay")
ue_blueprint_add_call_function_node(asset_path="...", function_name="PrintString")

# 4. Connect pins
ue_blueprint_connect_pins(asset_path="...",
    source_node_guid="<beginplay-guid>", source_pin_name="then",
    target_node_guid="<print-guid>", target_pin_name="execute")

# 5. Compile and save
ue_blueprint_compile_save(asset_path="...", save=true)
```

## Blueprint Spec (Declarative, Stage 11B)

```
# Export existing BP as spec
ue_blueprint_export_spec(asset_path="/Game/Blueprints/MyBPA")
# → returns {blueprint, nodes, edges}

# Create new BP from spec
ue_blueprint_apply_spec(spec={
    "blueprint": {"name": "MySpecBP", "parent_class": "Actor"},
    "nodes": [
        {"id": "beginplay", "type": "event", "event_name": "ReceiveBeginPlay"},
        {"id": "print", "type": "call_function", "function_name": "PrintString", "params": {"InString": "Hello"}}
    ],
    "edges": [
        {"from_node": "beginplay", "from_pin": "then", "to_node": "print", "to_pin": "execute"}
    ]
})
```

## Widget Tree Editing

```
# Create Widget Blueprint
ue_create_widget_blueprint(name="MyHUD", path="/Game/UI")

# Add child widgets
ue_widget_add_child(asset_path="/Game/UI/MyHUD", widget_class="CanvasPanel")
ue_widget_add_child(asset_path="/Game/UI/MyHUD", widget_class="TextBlock",
    parent_widget="CanvasPanel_0", widget_name="TitleText")

# Set properties
ue_widget_set_property(asset_path="/Game/UI/MyHUD",
    widget_name="TitleText", property_name="Text", value="Hello World")

# Remove child
ue_widget_remove_child(asset_path="/Game/UI/MyHUD", widget_name="TitleText")
```

## Transaction Wrapping

```
ue_begin_transaction(description="Spawn enemies")
ue_spawn_actor(class_name="BP_Enemy", location={x:0, y:0, z:100})
ue_spawn_actor(class_name="BP_Enemy", location={x:200, y:0, z:100})
ue_end_transaction()
# Rollback if needed: ue_undo()
```

## Material Parameter Editing

```
ue_material_set_scalar_param(asset_path="/Game/Materials/MI_Character",
    param_name="Roughness", value=0.5)
ue_material_set_vector_param(asset_path="/Game/Materials/MI_Character",
    param_name="BaseColor", r=1.0, g=0.0, b=0.0, a=1.0)
ue_material_set_texture_param(asset_path="/Game/Materials/MI_Character",
    param_name="DiffuseMap", texture_path="/Game/Textures/T_Character_D")
```

## Diagnostics

```
# Check bridge health
ue_get_bridge_runtime_status
# Returns: server_status, client_connected, last_error_code, transport_mode, etc.

# Get full action registry
ue_get_mcp_config
# Returns: actions[], count, mode, token_enabled, version
```
""",
    },
    "ue://resources/blueprint-spec": {
        "name": "Blueprint Spec Reference",
        "description": "11A/11B spec constraints, apply_spec/export_spec format, round-trip compatibility",
        "mimeType": "text/markdown",
        "content": """# Blueprint Spec Reference (Stage 11A/11B)

## Spec Format (apply_spec input)

```json
{
  "spec": {
    "blueprint": {
      "name": "MyBP",          // required
      "parent_class": "Actor", // default: "Actor"
      "path": "/Game/Blueprints"
    },
    "nodes": [
      {
        "id": "beginplay",               // required: unique string ID
        "type": "event",                  // "event" or "call_function"
        "event_name": "ReceiveBeginPlay" // for type=event
      },
      {
        "id": "print",
        "type": "call_function",
        "function_name": "PrintString",  // for type=call_function
        "params": {                       // optional
          "InString": "Hello World"
        }
      }
    ],
    "edges": [
      {
        "from_node": "beginplay",   // node ID reference
        "from_pin": "then",         // output pin name
        "to_node": "print",         // node ID reference
        "to_pin": "execute"         // input pin name
      }
    ]
  }
}
```

## Export Format (export_spec output)

```json
{
  "blueprint": {"name": "...", "parent_class": "Actor", "path": "..."},
  "nodes": [{"id": "...", "type": "...", "event_name": "...", "function_name": "..."}],
  "edges": [{"from_node": "...", "from_pin": "...", "to_node": "...", "to_pin": "..."}]
}
```

## Current Capability Boundaries

- **Blueprint type**: Actor Blueprint only
- **Graph**: EventGraph only (no function graphs, construction script)
- **Node types**: Event nodes + CallFunction nodes (white-list)
- **Spec IDs**: apply_spec and export_spec use the same `from_node`/`to_node` field names (round-trip compatible after Stage 11B fix #20)
- **No deletions**: Spec apply is incremental (adds nodes/edges), does not remove existing nodes

## Round-Trip Workflow

```
apply_spec(spec1) → export_spec → spec2
spec1.from_node == spec2.from_node  // IDs preserved
spec1.to_node   == spec2.to_node    // round-trip compatible
```

## Common Pitfalls

1. Node IDs must be unique within a spec
2. Edge `from_node`/`to_node` must reference node IDs declared in the same spec
3. `function_name` must be a `BlueprintCallable` function
4. Pin names are case-sensitive and must match UE's internal naming
5. Compile after apply (use `blueprint_compile_save`)
""",
    },
}


def get_static_resource(uri: str) -> types.ReadResourceResult | None:
    """Return a static resource by URI, or None if not found."""
    entry = RESOURCE_REGISTRY.get(uri)
    if entry is None:
        return None
    return types.ReadResourceResult(
        contents=[
            types.TextResourceContents(
                uri=uri,
                mimeType=entry["mimeType"],
                text=entry["content"],
            )
        ]
    )


def list_static_resources() -> list[types.Resource]:
    """Return all static resource descriptors."""
    return [
        types.Resource(
            uri=uri,
            name=entry["name"],
            description=entry["description"],
            mimeType=entry["mimeType"],
        )
        for uri, entry in RESOURCE_REGISTRY.items()
    ]
