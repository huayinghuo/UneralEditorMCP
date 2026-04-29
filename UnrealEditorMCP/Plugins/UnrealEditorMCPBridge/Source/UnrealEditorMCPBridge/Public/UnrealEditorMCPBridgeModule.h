#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FMCPBridgeServer;

// UE 插件模块入口
// 负责 FMCPBridgeServer 的创建/销毁，以及通过 OnBeginFrame 驱动其处理请求
class FUnrealEditorMCPBridgeModule : public IModuleInterface
{
public:
	// 模块加载：创建桥接服务器并注册 OnBeginFrame 回调
	virtual void StartupModule() override;
	// 模块卸载：注销回调、停止服务器、释放资源
	virtual void ShutdownModule() override;

private:
	// 引擎主循环每帧回调 → 驱动 ProcessPendingRequests 在 Game Thread 执行
	void OnBeginFrame();

	TUniquePtr<FMCPBridgeServer> Server;   // 桥接服务实例
	FDelegateHandle BeginFrameHandle;      // OnBeginFrame 注册句柄，用于 Shutdown 时注销
};
