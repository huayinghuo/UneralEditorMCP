# UnrealEditorMCP — Agent Coding Guide

## Quick Reference

| 项目 | 值 |
|------|-----|
| UE 版本 | 5.3（`E:\UE_5.3\`） |
| 项目文件 | `E:\UEProject\UnrealEditorMCP\UnrealEditorMCP\UnrealEditorMCP.uproject` |
| 插件 | `UnrealEditorMCP\Plugins\UnrealEditorMCPBridge\` |
| MCP 端口 | TCP 9876 |
| Handler 数 | 48（Read 21 / Write 26 / Dangerous 1） |
| 计划文档 | `.ai/plan/plan.md`（内层） + 外部主计划 `C:\Users\萤火\.local\share\kilo\plans\1777390210726-swift-squid.md` |

---

## 1. 编译

### 编译命令

从仓库根 `E:\UEProject\UnrealEditorMCP` 执行：

```powershell
E:\UE_5.3\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe `
  UnrealEditorMCPEditor Win64 Development `
  "-project=E:\UEProject\UnrealEditorMCP\UnrealEditorMCP\UnrealEditorMCP.uproject"
```

**预期输出**: 4 actions, ~15s, 0 errors.

### 编译注意事项

**必须先关闭 UE Editor**（否则 DLL 被锁定，链接失败）。检查方式：

```powershell
Get-Process -Name "UnrealEditor*" -ErrorAction SilentlyContinue
```

若有进程，先 `Stop-Process -Name "UnrealEditor"` 或手动关闭。

### UE 5.3 API 差异（已踩坑）

| 问题 | UE 5.3 正确写法 |
|------|----------------|
| `FMemberReference::GetMemberName(FName&)` | `FName Name = Ref.GetMemberName();`（返回值，非 out 参数） |
| `FUNC_Native` | 不存在，用 `FUNC_BlueprintCallable` 替代 |
| `FindObject<UClass>` | 用 `FindFirstObject<UClass>` |

### Module 依赖（Build.cs 已包含）

无需额外添加，当前 `UnrealEditorMCPBridge.Build.cs` 已包含：`Kismet`, `BlueprintGraph`, `AssetRegistry`, `UMG`, `UMGEditor`, `MaterialEditor`, `PythonScriptPlugin`, `Sockets`, `Networking`, `Json`, `JsonUtilities`.

---

## 2. 测试

### 2.1 启动 UE Editor

```powershell
Start-Process -FilePath "E:\UE_5.3\Engine\Binaries\Win64\UnrealEditor.exe" `
  -ArgumentList '"E:\UEProject\UnrealEditorMCP\UnrealEditorMCP\UnrealEditorMCP.uproject"' `
  -WindowStyle Minimized
```

**加载时间**: 30-60 秒。

### 2.2 等待 MCP Bridge 就绪

```powershell
for ($i=0; $i -lt 60; $i++) {
  try {
    $c = New-Object System.Net.Sockets.TcpClient('127.0.0.1', 9876)
    $c.Close()
    Write-Host "READY on port 9876"
    break
  } catch {
    Start-Sleep -Seconds 3
  }
}
```

### 2.3 发送 MCP 命令（PowerShell 直接 TCP）

此环境 **Python 不可用**（Windows Store stub），推荐 PowerShell 直连 TCP：

```powershell
$bridgeHost = "127.0.0.1"
$port = 9876

function Send-Request($action, $payload) {
    $req = @{ 
        id = (Get-Random -Maximum 99999).ToString()
        action = $action
        payload = $payload 
    }
    $json = (ConvertTo-Json -Compress $req) + "`n"
    
    $s = New-Object System.Net.Sockets.TcpClient($bridgeHost, $port)
    $stream = $s.GetStream()
    $writer = New-Object System.IO.StreamWriter($stream)
    $writer.Write($json)
    $writer.Flush()
    $reader = New-Object System.IO.StreamReader($stream)
    $line = $reader.ReadLine()
    $s.Close()
    return ConvertFrom-Json $line
}

# 示例
$r = Send-Request "ping"
Write-Host "ping ok=$($r.ok)"
```

**注意**: `$host` 是 PowerShell 只读变量，必须用 `$bridgeHost`。

### 2.4 测试脚本位置

| 文件 | 用途 |
|------|------|
| `UnrealEditorMCP\Tools\MCPBridgeServer\tests\test_bridge.py` | 基础联通测试（Python, 需可用） |
| `UnrealEditorMCP\Tools\MCPBridgeServer\tests\test_bp_ops.py` | Blueprint 变量/函数测试 |
| `UnrealEditorMCP\Tools\MCPBridgeServer\tests\test_stage11a.py` | 阶段 11A 验收测试（Python, 需可用） |
| `UnrealEditorMCP\Tools\MCPBridgeServer\tests\test_stage11a.ps1` | 阶段 11A 验收测试（PowerShell, 推荐） |

### 2.5 验收测试检查清单

每次新增 Handler 后执行：

1. **编译通过** — 4 actions, 0 errors
2. **ping 联通** — ok=True
3. **正向链路** — 新 action 的典型调用路径 ok=True、返回结构符合预期
4. **错误路径** — 至少 3 个非法输入分别返回 ok=False + 正确错误码

---

## 3. 代码模式

### 3.1 新增 Handler 的完整步骤

1. **声明** `Public/Handlers/MCPXxxHandlers.h` — 继承 `IMCPBridgeHandler`，实现 3 个虚函数
2. **实现** `Private/Handlers/MCPXxxHandlers.cpp` — Execute 方法
3. **注册** `Private/MCPBridgeServer.cpp` — `#include` + `RegisterHandler(MakeShareable(new ...))`
4. **Schema** `Tools/.../tool_schemas.py` — `"action_name": { name, description, inputSchema }`

### 3.2 Handler 模板

```cpp
// === 头文件 ===
class FMCPDoSomethingHandler : public IMCPBridgeHandler
{
public:
    virtual FString GetActionName() const override { return TEXT("my_action"); }
    virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Write; }
    virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, 
                         FString& OutErrorCode, FString& OutErrorMessage) override;
};

// === 实现文件 ===
bool FMCPDoSomethingHandler::Execute(...)
{
    // 1. 参数校验
    FString RequiredParam;
    if (!Payload.IsValid() || !Payload->TryGetStringField(TEXT("key"), RequiredParam))
    {
        MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"), TEXT("payload.key is required"), 
                                             OutErrorCode, OutErrorMessage);
        return false;
    }
    
    // 2. 加载/查询数据
    UBlueprint* BP = LoadObject<UBlueprint>(nullptr, *AssetPath);
    if (!BP) { ... return false; }
    
    // 3. 执行操作前调用 Modify()（写操作必须）
    BP->Modify();
    
    // 4. 执行实际操作
    // ...
    
    // 5. 标记修改
    FBlueprintEditorUtils::MarkBlueprintAsModified(BP);
    
    // 6. 构建结构化响应
    OutResult = MakeShareable(new FJsonObject());
    OutResult->SetStringField(TEXT("key"), TEXT("value"));
    return true;
}
```

### 3.3 关键规则

- **写操作必须先调用 `BP->Modify()`**（undo 跟踪），操作后调用 `MarkBlueprintAsModified` / `MarkBlueprintAsStructurallyModified`
- **错误码统一用 `MCPBridgeHelpers::BuildErrorResponse(code, msg, ...)`**，不要直接用字符串赋值
- **每个错误路径都必须设 `OutErrorCode` + `OutErrorMessage` 并返回 false**
- **新增错误码时同步更新 `.ai/plan/plan.md` 错误码表**
- **Graph 节点操作**使用 `NewObject<UK2Node_Xxx>(Graph)`，设置属性，`AllocateDefaultPins()`，`Graph->AddNode(Node, false, false)`

### 3.4 Blueprint Graph 编辑专用模式

```cpp
// 创建事件节点
UK2Node_Event* Node = NewObject<UK2Node_Event>(Graph);
Node->EventReference.SetExternalMember(MemberName, ParentClass);
Node->CreateNewGuid();
Node->AllocateDefaultPins();
Graph->AddNode(Node, false, false);

// 创建函数调用节点
UK2Node_CallFunction* Node = NewObject<UK2Node_CallFunction>(Graph);
Node->SetFromFunction(Function);
Node->CreateNewGuid();
Node->AllocateDefaultPins();
Graph->AddNode(Node, false, false);

// 连接引脚
const UEdGraphSchema* Schema = Graph->GetSchema();
Schema->TryCreateConnection(SourcePin, TargetPin);

// 查找节点/引脚
UEdGraphNode* Node = FindNodeByGuid(Graph, NodeGuidString);
UEdGraphPin* Pin = FindPin(Node, PinName, EGPD_Output); // or EGPD_Input
```

---

## 4. 常见问题

### 4.1 Stop() 重入崩溃

**现象**: `EXCEPTION_STACK_OVERFLOW` in `FMCPBridgeServer::Stop()`

**原因**: `Thread->WaitForCompletion()` 期间 UE 模块关闭再次触发 Stop()，无防重入守卫。

**已修复** (`MCPBridgeServer.cpp:109`): 添加 `if (bStopping) return;` 作为首行。

### 4.2 Python 不可用

当前环境 Python 无法正常执行。测试必须通过 PowerShell 直连 TCP socket。

### 4.3 文件编码

- C++: `.editorconfig` 指定 CRLF, tab 缩进
- Python/Markdown/JSON: space 缩进
- 所有文本文件 UTF-8 (无 BOM)

### 4.4 Git push 被透明代理阻断

**现象**: `git push` 报 `Recv failure: Connection was reset`。

**原因**: 系统网络有透明代理拦截 HTTPS。

**解决**: 推送时绕过代理：
```powershell
git -c http.proxy="" push origin master
```

- C++: `.editorconfig` 指定 CRLF, tab 缩进
- Python/Markdown/JSON: space 缩进
- 所有文本文件 UTF-8 (无 BOM)

---

## 5. 文件结构速查

```
UnrealEditorMCP/
├── .ai/                          # 项目文档
│   ├── plan/plan.md              # 实施状态 + Handler 清单 + 错误码表
│   ├── plan/plan_index.md        # 当前阶段摘要
│   ├── plan/plan_log.md          # 变更时间线
│   ├── dev/dev_last.md           # 最后一次操作记录
│   └── log/                      # 操作日志（按日期归档）
├── .kilo/                        # Kilo agent 配置
├── README.md                     # 用户文档
├── .editorconfig / .gitattributes / .gitignore
│
├── UnrealEditorMCP/              # UE 项目
│   ├── UnrealEditorMCP.uproject
│   ├── Config/DefaultUnrealEditorMCPBridge.ini
│   ├── Plugins/UnrealEditorMCPBridge/
│   │   ├── Source/UnrealEditorMCPBridge/
│   │   │   ├── Public/
│   │   │   │   ├── MCPBridgeServer.h         # TCP Server (FRunnable)
│   │   │   │   ├── MCPBridgeHandler.h         # Handler 接口
│   │   │   │   ├── MCPBridgeDispatcher.h      # 表驱动分发 + token 校验
│   │   │   │   ├── MCPBridgeHelpers.h         # 通用辅助（反射/查找/Parse）
│   │   │   │   ├── MCPBlueprintGraphHelpers.h # BP 图编辑辅助
│   │   │   │   └── Handlers/                  # 10 个 Handler 头文件
│   │   │   ├── Private/
│   │   │   │   ├── MCPBridgeServer.cpp         # Server + 所有 Handler 注册
│   │   │   │   ├── MCPBlueprintGraphHelpers.cpp
│   │   │   │   └── Handlers/                  # 10 个 Handler 实现
│   │   │   └── UnrealEditorMCPBridge.Build.cs  # 模块依赖
│   │   └── Binaries/Win64/                     # 编译输出
│   └── Tools/MCPBridgeServer/
│       ├── src/mcp_bridge_server/
│       │   ├── server.py           # MCP stdio Server
│       │   ├── bridge_client.py    # TCP 客户端（自动重连 + ID 校验）
│       │   └── tool_schemas.py     # 43 个 Tool Schema 注册表
│       └── tests/                  # Python 测试脚本
```

---

## 6. 操作日志规范

每次编码会话后更新以下文件：

1. `.ai/dev/dev_last.md` — 覆盖为本次操作摘要
2. `.ai/plan/plan_log.md` — 追加日期行
3. `.ai/log/log.md` — 追加日志索引
4. `.ai/log/YYYY/MM/YYYY-MM-DD-NNN.md` — 新建详细操作日志
5. 若新增 Handler / 错误码，同步 `.ai/plan/plan.md`
