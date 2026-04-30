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

/**
 * MCP 桥接 TCP 服务器（传输壳层）
 * 继承 FRunnable，运行在独立工作线程中
 * 架构：
 *   网络线程 ──收包/入队──→ RequestQueue ──→ Game Thread (Dispatcher.Dispatch) ──→ ResponseQueue ──→ 网络线程 ──发回
 * 连接模型：单客户端独占 —— 同一时刻只允许一个客户端连接，第二客户端会被拒绝（返回 CLIENT_ALREADY_CONNECTED）
 * 状态面：EMCPBridgeServerStatus 反映实时状态，LastErrorCode/LastErrorMessage 记录最近一次故障详情
 */
class FMCPBridgeServer : public FRunnable
{
public:
	FMCPBridgeServer();
	virtual ~FMCPBridgeServer();

	/**
	 * 启动服务器，绑定指定端口并创建工作线程
	 * @param Port 监听端口号
	 * @return true=线程创建成功，false=创建失败（不代表 Bind/Listen 成功）
	 */
	bool Start(int32 Port);

	// FRunnable 接口 —— 均在网络线程上下文调用
	virtual bool Init() override;
	virtual uint32 Run() override;
	virtual void Stop() override;
	virtual void Exit() override;

	/** 在 Game Thread（OnBeginFrame）调用，消费请求队列并通过 Dispatcher 分发 */
	void ProcessPendingRequests();

	/** @return 服务器当前运行状态（线程安全） */
	EMCPBridgeServerStatus GetStatus() const;
	/** @return 服务是否处于可接受连接的就绪状态 */
	bool IsListening() const;
	/** @return 是否有客户端已连接 */
	bool IsClientConnected() const;
	/** @return 监听端口号 */
	int32 GetServerPort() const { return ServerPort; }

	/** 记录最近一次错误信息（网络线程写入，Game Thread 可读取） */
	void SetLastError(const FString& Code, const FString& Message);
	/** @return 最近一次错误码（非 const：内部加锁修改 FCriticalSection 状态） */
	FString GetLastErrorCode();
	/** @return 最近一次错误描述（非 const：内部加锁修改 FCriticalSection 状态） */
	FString GetLastErrorMessage();

	/** 设置 token（空 = 不校验写操作）；仅在 Start 前调用 */
	void SetToken(const FString& InToken) { Dispatcher.SetToken(InToken); }
	FString GetToken() const { return Dispatcher.GetToken(); }

	/** 返回所有已注册 action 及其分类信息 */
	TArray<TSharedPtr<FJsonValue>> GetAllActionsInfo() { return Dispatcher.GetAllActionsInfo(); }

private:
	/** 向 Dispatcher 注册所有已实现的 Handler */
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
	FCriticalSection ClientSocketCS;                    // 保护 ClientSocket 的读写
	FCriticalSection LastErrorCS;                       // 保护 LastErrorCode/LastErrorMessage 的读写
	TAtomic<EMCPBridgeServerStatus> ServerStatus;       // 线程安全状态，网络线程写 / Game Thread 读

	// --- 运行时诊断字段（网络线程写入，Game Thread 可读）---
	FString LastErrorCode;       // 最近一次错误的错误码（如 BIND_FAILED, LISTEN_FAILED）
	FString LastErrorMessage;    // 最近一次错误的描述详情
};
