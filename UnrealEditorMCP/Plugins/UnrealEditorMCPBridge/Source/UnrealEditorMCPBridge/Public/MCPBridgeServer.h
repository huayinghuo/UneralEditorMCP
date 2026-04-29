#pragma once

#include "CoreMinimal.h"
#include "HAL/Runnable.h"
#include "Sockets.h"
#include "Interfaces/IPv4/IPv4Endpoint.h"
#include "Containers/Queue.h"
#include "MCPBridgeTypes.h"
#include "MCPBridgeDispatcher.h"

// 服务器状态枚举
enum class EMCPBridgeServerStatus : uint8
{
	Unstarted,   // 尚未启动
	Listening,   // Bind+Listen 成功，等待客户端连接
	Connected,   // 有客户端已连接
	Error,       // Bind/Listen 失败
	Stopped      // 已停止
};

// MCP 桥接 TCP 服务器（传输壳层）
// 继承 FRunnable，运行在独立工作线程中
// 架构：
//   网络线程 ──收包/入队──→ RequestQueue ──→ Game Thread (Dispatcher.Dispatch) ──→ ResponseQueue ──→ 网络线程 ──发回
// Handler 注册：由构造函数调用 RegisterHandlers()，将 6 个 Handler 注册到 Dispatcher
// 后续扩展只需新增 Handler 类并在 RegisterHandlers 中注册
class FMCPBridgeServer : public FRunnable
{
public:
	FMCPBridgeServer();
	virtual ~FMCPBridgeServer();

	// 启动服务器，绑定指定端口并创建工作线程
	bool Start(int32 Port);

	// FRunnable 接口 —— 均在网络线程上下文调用
	virtual bool Init() override;
	virtual uint32 Run() override;
	virtual void Stop() override;
	virtual void Exit() override;

	// 在 Game Thread（OnBeginFrame）调用，消费请求队列并通过 Dispatcher 分发
	void ProcessPendingRequests();

	// 获取服务器当前运行状态（线程安全）
	EMCPBridgeServerStatus GetStatus() const;
	bool IsListening() const;

	// 设置 token（空 = 不校验写操作）；仅在 Start 前调用
	void SetToken(const FString& InToken) { Dispatcher.SetToken(InToken); }
	FString GetToken() const { return Dispatcher.GetToken(); }

	// 返回所有已注册 action 及其分类信息
	TArray<TSharedPtr<FJsonValue>> GetAllActionsInfo() { return Dispatcher.GetAllActionsInfo(); }

private:
	// 向 Dispatcher 注册所有已实现的 Handler（6 个）
	void RegisterHandlers();

	// 将响应序列化为 JSON Lines 格式（末尾 \n），通过 socket 发送
	bool SendResponse(FSocket* Client, const FMCPPendingResponse& Response);

	// --- Handler 调度器 ---
	FMCPBridgeDispatcher Dispatcher;  // 表驱动分发，替代原来 if (Action == ...) 链

	// --- 网络资源 ---
	FSocket* ListenerSocket;
	FSocket* ClientSocket;
	FRunnableThread* Thread;
	TAtomic<bool> bStopping;
	int32 ServerPort;

	// --- 线程间通信队列（Mpsc = 多生产者 / 单消费者）---
	TQueue<FMCPPendingRequest, EQueueMode::Mpsc> RequestQueue;
	TQueue<FMCPPendingResponse, EQueueMode::Mpsc> ResponseQueue;

	// --- 同步原语 ---
	FCriticalSection ClientSocketCS;
	TAtomic<EMCPBridgeServerStatus> ServerStatus;  // 线程安全状态，网络线程写 / Game Thread 读
};
