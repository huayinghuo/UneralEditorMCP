#include "UnrealEditorMCPBridgeModule.h"
#include "MCPBridgeServer.h"
#include "Misc/CoreDelegates.h"
#include "Misc/ConfigCacheIni.h"
#include "HAL/PlatformProcess.h"

DEFINE_LOG_CATEGORY_STATIC(LogMCPBridge, Log, All);

#define LOCTEXT_NAMESPACE "FUnrealEditorMCPBridgeModule"

void FUnrealEditorMCPBridgeModule::StartupModule()
{
	UE_LOG(LogMCPBridge, Log, TEXT("UnrealEditorMCPBridge: StartupModule"));

	// 从插件 Config/DefaultUnrealEditorMCPBridge.ini 读取配置
	FString ConfigFile = FPaths::ProjectPluginsDir() / TEXT("UnrealEditorMCPBridge/Config/DefaultUnrealEditorMCPBridge.ini");
	int32 Port = 9876;
	GConfig->GetInt(TEXT("UnrealEditorMCPBridge"), TEXT("Port"), Port, *ConfigFile);

	Server = MakeUnique<FMCPBridgeServer>();

	// Token 配置：非空时启用校验
	FString Token;
	GConfig->GetString(TEXT("UnrealEditorMCPBridge"), TEXT("Token"), Token, *ConfigFile);
	if (!Token.IsEmpty())
	{
		Server->SetToken(Token);
		UE_LOG(LogMCPBridge, Log, TEXT("Token auth enabled"));
	}

	Server->Start(Port);

	// 短暂轮询确认 Bind/Listen 成功（线程创建后 1 秒内）
	for (int32 i = 0; i < 20; ++i)
	{
		if (Server->IsListening())
		{
			UE_LOG(LogMCPBridge, Log, TEXT("MCP Bridge server ready on port %d"), Port);
			break;
		}
		FPlatformProcess::Sleep(0.05f);
	}

	if (!Server->IsListening())
	{
		UE_LOG(LogMCPBridge, Warning, TEXT("MCP Bridge server not ready (status=%d)"), static_cast<int32>(Server->GetStatus()));
	}

	BeginFrameHandle = FCoreDelegates::OnBeginFrame.AddRaw(
		this, &FUnrealEditorMCPBridgeModule::OnBeginFrame
	);
}

void FUnrealEditorMCPBridgeModule::ShutdownModule()
{
	UE_LOG(LogMCPBridge, Log, TEXT("UnrealEditorMCPBridge: ShutdownModule"));

	// 注销引擎回调，防止悬空指针
	FCoreDelegates::OnBeginFrame.Remove(BeginFrameHandle);

	if (Server)
	{
		// 停止工作线程并释放所有 socket 资源
		Server->Stop();
		Server.Reset();
	}
}

void FUnrealEditorMCPBridgeModule::OnBeginFrame()
{
	// 每帧将请求队列中的消息在 Game Thread 中处理
	if (Server)
	{
		Server->ProcessPendingRequests();
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FUnrealEditorMCPBridgeModule, UnrealEditorMCPBridge)
