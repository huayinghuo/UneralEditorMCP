#include "MCPBridgeServer.h"
#include "Handlers/MCPQueryHandlers.h"
#include "Handlers/MCPActorHandlers.h"
#include "Handlers/MCPPythonHandler.h"
#include "Handlers/MCPWriteHandlers.h"
#include "Handlers/MCPTransactionHandlers.h"
#include "Handlers/MCPBlueprintHandlers.h"
#include "Handlers/MCPViewportHandlers.h"
#include "Handlers/MCPMaterialHandlers.h"
#include "Handlers/MCPWidgetHandlers.h"
#include "Handlers/MCPDirtyHandlers.h"
#include "Handlers/MCPBlueprintGraphHandlers.h"
#include "Handlers/MCPBlueprintSpecHandlers.h"
#include "Handlers/MCPBlueprintAdvancedHandlers.h"
#include "Handlers/MCPBlueprintUtilityHandlers.h"
#include "Handlers/MCPPIERuntimeHandlers.h"
#include "Handlers/MCPEnhancedInputHandlers.h"
#include "Handlers/MCPGameplayTagHandlers.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"
#include "Dom/JsonObject.h"
#include "HAL/PlatformProcess.h"

DEFINE_LOG_CATEGORY_STATIC(LogMCPBridgeServer, Log, All);

static const int32 DefaultBridgePort = 9876;

FMCPBridgeServer::FMCPBridgeServer()
	: ListenerSocket(nullptr)
	, ClientSocket(nullptr)
	, Thread(nullptr)
	, bStopping(false)
	, ServerPort(DefaultBridgePort)
	, ServerStatus(EMCPBridgeServerStatus::Unstarted)
{
	RegisterHandlers();
}

FMCPBridgeServer::~FMCPBridgeServer()
{
	Stop();
}

void FMCPBridgeServer::RegisterHandlers()
{
	// 查询类（8 个 — 含诊断查询）
	Dispatcher.RegisterHandler(MakeShareable(new FMCPPingHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPGetEditorInfoHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPGetProjectInfoHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPGetWorldStateHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPListAssetsHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPGetAssetInfoHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPGetMCPConfigHandler(this)));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPGetBridgeRuntimeStatusHandler(this)));
	// Actor 类（4 个）
	Dispatcher.RegisterHandler(MakeShareable(new FMCPGetSelectedActorsHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPListLevelActorsHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPGetActorPropertyHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPGetComponentPropertyHandler()));
	// Python（1 个）
	Dispatcher.RegisterHandler(MakeShareable(new FMCPExecutePythonHandler()));
	// 写操作（6 个）
	Dispatcher.RegisterHandler(MakeShareable(new FMCPSpawnActorHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPLevelSetActorTransformHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPActorSetPropertyHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPSaveCurrentLevelHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPDeleteActorHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPSetComponentPropertyHandler()));
	// 事务（4 个）
	Dispatcher.RegisterHandler(MakeShareable(new FMCPBeginTransactionHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPEndTransactionHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPUndoHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPRedoHandler()));
	// Blueprint（6 个）
	Dispatcher.RegisterHandler(MakeShareable(new FMCPListBlueprintsHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPGetBlueprintInfoHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPCreateBlueprintHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPAddBlueprintVariableHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPAddBlueprintFunctionHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPBlueprintAddComponentHandler()));
	// Blueprint Graph 编辑（6 个）
	Dispatcher.RegisterHandler(MakeShareable(new FMCPCreateActorBlueprintClassHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPGetBlueprintEventGraphInfoHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPBlueprintAddEventNodeHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPBlueprintAddCallFunctionNodeHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPBlueprintConnectPinsHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPCompileSaveBlueprintHandler()));
	// Blueprint Spec 编辑（2 个）
	Dispatcher.RegisterHandler(MakeShareable(new FMCPApplyBlueprintSpecHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPExportBlueprintSpecHandler()));
	// Blueprint Advanced 编辑（7 个）
	Dispatcher.RegisterHandler(MakeShareable(new FMCPAddNodeByClassHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPAddVariableNodeHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPSetPinDefaultHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPGetFunctionSignatureHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPRemoveNodeHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPDisconnectPinsHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPRemoveVariableHandler()));
	// Blueprint Utility（3 个）
	Dispatcher.RegisterHandler(MakeShareable(new FMCPBlueprintSearchFunctionsHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPBlueprintSetVariableDefaultHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPBlueprintSetComponentDefaultHandler()));
	// Blueprint CDO Property（4 个 — 阶段 19A）
	Dispatcher.RegisterHandler(MakeShareable(new FMCPBlueprintGetCDOPropertyHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPBlueprintSetCDOPropertyHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPBlueprintAddCDOArrayHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPBlueprintRemoveCDOArrayHandler()));
	// GameplayTag（3 个 — 阶段 19B）
	Dispatcher.RegisterHandler(MakeShareable(new FMCPCreateGameplayTagHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPListGameplayTagsHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPSearchGameplayTagsHandler()));
	// PIE Runtime（5 个）
	Dispatcher.RegisterHandler(MakeShareable(new FMCPPIEStartHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPPIEStopHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPPIEIsRunningHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPGetActorStateHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPSetLevelDefaultPawnHandler()));
	// Enhanced Input（14 个 — Stage 18C）
	Dispatcher.RegisterHandler(MakeShareable(new FMCPSearchInputActionsHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPCreateInputActionHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPGetInputActionInfoHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPDeleteInputActionHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPSearchIMCHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPCreateIMCHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPGetIMCInfoHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPDeleteIMCHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPAddMappingHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPRemoveMappingHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPSetMappingActionHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPSetMappingKeyHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPBPEnhancedActionHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPBPIMCNodeHandler()));
	// Viewport（1 个）
	Dispatcher.RegisterHandler(MakeShareable(new FMCPViewportScreenshotHandler()));
	// Material（5 个）
	Dispatcher.RegisterHandler(MakeShareable(new FMCPListMaterialsHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPGetMaterialInfoHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPSetMaterialScalarParamHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPSetMaterialVectorParamHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPSetMaterialTextureParamHandler()));
	// Widget（17 个 — 含 Stage 16 完整深化）
	Dispatcher.RegisterHandler(MakeShareable(new FMCPListWidgetsHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPGetWidgetInfoHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPCreateWidgetBlueprintHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPWidgetAddChildHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPWidgetRemoveChildHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPWidgetSetPropertyHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPWidgetGetPropertySchemaHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPWidgetGetSlotSchemaHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPWidgetFindHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPWidgetSetRootHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPWidgetReparentHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPWidgetReorderChildHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPWidgetRenameHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPWidgetSetSlotPropertyHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPWidgetDuplicateHandler()));
	Dispatcher.RegisterHandler(MakeShareable(new FMCPWidgetWrapWithPanelHandler()));
	// Dirty（1 个）
	Dispatcher.RegisterHandler(MakeShareable(new FMCPGetDirtyPackagesHandler()));
}

bool FMCPBridgeServer::Start(int32 Port)
{
	ServerPort = Port;
	bStopping = false;
	Thread = FRunnableThread::Create(this, TEXT("MCPBridgeServer"), 0, TPri_Normal);
	return Thread != nullptr;
}

void FMCPBridgeServer::Stop()
{
	if (bStopping) return;
	bStopping = true;
	ServerStatus = EMCPBridgeServerStatus::Stopped;  // 标记停止

	if (Thread)
	{
		Thread->WaitForCompletion();
		delete Thread;
		Thread = nullptr;
	}

	FScopeLock Lock(&ClientSocketCS);
	if (ClientSocket)
	{
		ClientSocket->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ClientSocket);
		ClientSocket = nullptr;
	}

	if (ListenerSocket)
	{
		ListenerSocket->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ListenerSocket);
		ListenerSocket = nullptr;
	}
}

bool FMCPBridgeServer::Init()
{
	return true;
}

uint32 FMCPBridgeServer::Run()
{
	ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	if (!SocketSubsystem)
	{
		UE_LOG(LogMCPBridgeServer, Error, TEXT("Failed to get socket subsystem"));
		ServerStatus = EMCPBridgeServerStatus::Error;
		SetLastError(TEXT("SOCKET_SUBSYSTEM_FAILED"), TEXT("Failed to acquire platform socket subsystem"));
		return 1;
	}

	ListenerSocket = SocketSubsystem->CreateSocket(NAME_Stream, TEXT("MCPBridgeListener"), false);
	if (!ListenerSocket)
	{
		UE_LOG(LogMCPBridgeServer, Error, TEXT("Failed to create listener socket"));
		ServerStatus = EMCPBridgeServerStatus::Error;
		SetLastError(TEXT("CREATE_SOCKET_FAILED"), TEXT("Failed to create listener socket"));
		return 1;
	}

	ListenerSocket->SetNonBlocking(true);
	ListenerSocket->SetReuseAddr(true);

	FIPv4Address BindAddr(127, 0, 0, 1);
	FIPv4Endpoint Endpoint(BindAddr, ServerPort);

	if (!ListenerSocket->Bind(*Endpoint.ToInternetAddr()))
	{
		UE_LOG(LogMCPBridgeServer, Error, TEXT("Failed to bind to %s:%d"), *BindAddr.ToString(), ServerPort);
		ServerStatus = EMCPBridgeServerStatus::Error;
		SetLastError(TEXT("BIND_FAILED"), FString::Printf(TEXT("Failed to bind to %s:%d"), *BindAddr.ToString(), ServerPort));
		return 1;
	}

	if (!ListenerSocket->Listen(1))
	{
		UE_LOG(LogMCPBridgeServer, Error, TEXT("Failed to listen on port %d"), ServerPort);
		ServerStatus = EMCPBridgeServerStatus::Error;
		SetLastError(TEXT("LISTEN_FAILED"), FString::Printf(TEXT("Failed to listen on port %d"), ServerPort));
		return 1;
	}

	UE_LOG(LogMCPBridgeServer, Log, TEXT("MCP Bridge listening on 127.0.0.1:%d"), ServerPort);
	ServerStatus = EMCPBridgeServerStatus::Listening;  // Bind+Listen 成功，服务就绪

	FString ReadBuffer;
	int32 PendingRequestCount = 0;
	double LastRequestTime = 0.0;

	while (!bStopping)
	{
		FSocket* ActiveClient = nullptr;
		{
			FScopeLock Lock(&ClientSocketCS);
			ActiveClient = ClientSocket;
		}

		bool bHasPendingConnection = false;
		if (ListenerSocket->HasPendingConnection(bHasPendingConnection) && bHasPendingConnection)
		{
			TSharedRef<FInternetAddr> RemoteAddr = SocketSubsystem->CreateInternetAddr();
			if (!ActiveClient)
			{
				// 无活跃客户端 → 正常接受连接
				FScopeLock Lock(&ClientSocketCS);
				ClientSocket = ListenerSocket->Accept(*RemoteAddr, TEXT("MCPBridgeClient"));
				if (ClientSocket)
				{
					UE_LOG(LogMCPBridgeServer, Log, TEXT("Client connected from %s"), *RemoteAddr->ToString(true));
					ServerStatus = EMCPBridgeServerStatus::Connected;  // 客户端已连接
					ReadBuffer.Empty();
				}
				{
					FScopeLock Lock2(&ClientSocketCS);
					ActiveClient = ClientSocket;
				}
			}
			else
			{
				// 已有活跃客户端 → 拒绝第二连接（单客户端独占模型）
				FSocket* RejectedSocket = ListenerSocket->Accept(*RemoteAddr, TEXT("MCPBridgeRejected"));
				if (RejectedSocket)
				{
					UE_LOG(LogMCPBridgeServer, Warning,
						TEXT("Rejected second client connection from %s (single-client model, '%s' already connected)"),
						*RemoteAddr->ToString(true), *ActiveClient->GetDescription());
					// 向被拒绝的客户端发送结构化错误后立即关闭
					FMCPPendingResponse RejectResp;
					RejectResp.Id = TEXT("");
					RejectResp.bOk = false;
					RejectResp.ErrorCode = TEXT("CLIENT_ALREADY_CONNECTED");
					RejectResp.ErrorMessage = TEXT("Server is single-client: another client is already connected. Please disconnect first.");
					SendResponse(RejectedSocket, RejectResp);
					RejectedSocket->Close();
					SocketSubsystem->DestroySocket(RejectedSocket);
				}
			}
		}

		if (ActiveClient)
		{
			uint8 Buffer[4096];
			int32 BytesRead = 0;

			if (ActiveClient->Wait(ESocketWaitConditions::WaitForRead, FTimespan::FromMilliseconds(50)))
			{
				if (ActiveClient->Recv(Buffer, sizeof(Buffer) - 1, BytesRead) && BytesRead > 0)
				{
					Buffer[BytesRead] = '\0';
					ReadBuffer += UTF8_TO_TCHAR(Buffer);
				}
				else if (BytesRead == 0)
				{
					UE_LOG(LogMCPBridgeServer, Log, TEXT("Client disconnected (zero bytes read)"));
					FScopeLock Lock(&ClientSocketCS);
					if (ClientSocket)
					{
						ClientSocket->Close();
						SocketSubsystem->DestroySocket(ClientSocket);
						ClientSocket = nullptr;
					}
					ReadBuffer.Empty();
					ServerStatus = EMCPBridgeServerStatus::Listening;
					MCPTransaction_AutoEndIfActive();
					// 丢弃旧连接的全部残留队列
					FMCPPendingRequest DummyReq;
					while (RequestQueue.Dequeue(DummyReq)) {}
					FMCPPendingResponse DummyResp;
					while (ResponseQueue.Dequeue(DummyResp)) {}
				}
			}

			int32 NewlinePos;
			while (ReadBuffer.FindChar(TEXT('\n'), NewlinePos))
			{
				FString Line = ReadBuffer.Left(NewlinePos).TrimStartAndEnd();
				ReadBuffer.RightChopInline(NewlinePos + 1);

				if (Line.IsEmpty())
				{
					continue;
				}

				TSharedPtr<FJsonObject> JsonObject;
				TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Line);

				if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
				{
					FString Id;
					JsonObject->TryGetStringField(TEXT("id"), Id);

					FString Action;
					JsonObject->TryGetStringField(TEXT("action"), Action);

					const TSharedPtr<FJsonObject>* PayloadPtr = nullptr;
					TSharedPtr<FJsonObject> Payload;
					if (JsonObject->TryGetObjectField(TEXT("payload"), PayloadPtr))
					{
						Payload = *PayloadPtr;
					}

					if (Action.IsEmpty())
					{
		// 请求超时检测：Game Thread 被弹窗阻塞时，Server 线程主动返回警告
			if (PendingRequestCount > 0 && ActiveClient && (FPlatformTime::Seconds() - LastRequestTime) > 5.0)
			{
				UE_LOG(LogMCPBridgeServer, Warning, TEXT("Request timeout (>5s) — Game Thread may be blocked by modal dialog"));
				FMCPPendingResponse TimeoutResp;
				TimeoutResp.Id = TEXT("");
				TimeoutResp.bOk = false;
				TimeoutResp.ErrorCode = TEXT("TIMEOUT");
				TimeoutResp.ErrorMessage = TEXT("Request timed out (>5s). UE Editor may be blocked by a modal dialog. Close the dialog and retry.");
				SendResponse(ActiveClient, TimeoutResp);
				// 清空队列防止积压
				FMCPPendingRequest DummyReq;
				while (RequestQueue.Dequeue(DummyReq)) {}
				PendingRequestCount = 0;
			}

			FMCPPendingResponse Resp;
						Resp.Id = Id;
						Resp.bOk = false;
						Resp.ErrorCode = TEXT("MISSING_ACTION");
						Resp.ErrorMessage = TEXT("Action field is required");
						SendResponse(ActiveClient, Resp);
					}
					else
					{
						FMCPPendingRequest Request;
						Request.Id = Id;
						Request.Action = Action;
						Request.Payload = Payload;
						RequestQueue.Enqueue(Request);
						PendingRequestCount++;
						LastRequestTime = FPlatformTime::Seconds();
					}
				}
				else
				{
					UE_LOG(LogMCPBridgeServer, Warning, TEXT("Failed to parse JSON line"));
					FMCPPendingResponse Resp;
					Resp.Id = TEXT("");
					Resp.bOk = false;
					Resp.ErrorCode = TEXT("PARSE_ERROR");
					Resp.ErrorMessage = TEXT("Invalid JSON");
					SendResponse(ActiveClient, Resp);
				}
			}

			FMCPPendingResponse Resp;
			while (ResponseQueue.Dequeue(Resp))
			{
				SendResponse(ActiveClient, Resp);
				PendingRequestCount = FMath::Max(0, PendingRequestCount - 1);
			}
		}

		FPlatformProcess::Sleep(0.01f);
	}

	return 0;
}

void FMCPBridgeServer::Exit()
{
}

EMCPBridgeServerStatus FMCPBridgeServer::GetStatus() const
{
	return ServerStatus;
}

bool FMCPBridgeServer::IsListening() const
{
	EMCPBridgeServerStatus Status = GetStatus();
	return Status == EMCPBridgeServerStatus::Listening || Status == EMCPBridgeServerStatus::Connected;
}

bool FMCPBridgeServer::IsClientConnected() const
{
	return GetStatus() == EMCPBridgeServerStatus::Connected;
}

void FMCPBridgeServer::SetLastError(const FString& Code, const FString& Message)
{
	FScopeLock Lock(&LastErrorCS);
	LastErrorCode = Code;
	LastErrorMessage = Message;
}

FString FMCPBridgeServer::GetLastErrorCode()
{
	FScopeLock Lock(&LastErrorCS);
	return LastErrorCode;
}

FString FMCPBridgeServer::GetLastErrorMessage()
{
	FScopeLock Lock(&LastErrorCS);
	return LastErrorMessage;
}

void FMCPBridgeServer::ProcessPendingRequests()
{
	// 从请求队列中取出所有待处理请求，通过 Dispatcher 表驱动分发
	FMCPPendingRequest Request;
	int32 ProcessedCount = 0;
	while (RequestQueue.Dequeue(Request))
	{
		TSharedPtr<FJsonObject> ResultObj;
		FString ErrorCode, ErrorMessage;
		bool bOk = Dispatcher.Dispatch(Request.Action, Request.Payload, ResultObj, ErrorCode, ErrorMessage);

		FMCPPendingResponse Response;
		Response.Id = Request.Id;
		Response.bOk = bOk;
		Response.ResultObj = ResultObj;
		Response.ErrorCode = ErrorCode;
		Response.ErrorMessage = ErrorMessage;
		ResponseQueue.Enqueue(Response);
		ProcessedCount++;
	}

	if (ProcessedCount > 0)
	{
		UE_LOG(LogMCPBridgeServer, Verbose, TEXT("Processed %d requests on game thread"), ProcessedCount);
	}
}

bool FMCPBridgeServer::SendResponse(FSocket* Client, const FMCPPendingResponse& Response)
{
	if (!Client)
	{
		return false;
	}

	TSharedPtr<FJsonObject> ResponseObj = MakeShareable(new FJsonObject());
	ResponseObj->SetStringField(TEXT("id"), Response.Id);
	ResponseObj->SetBoolField(TEXT("ok"), Response.bOk);

	if (Response.bOk && Response.ResultObj.IsValid())
	{
		ResponseObj->SetField(TEXT("result"), MakeShareable(new FJsonValueObject(Response.ResultObj)));
		ResponseObj->SetField(TEXT("error"), MakeShareable(new FJsonValueNull()));
	}
	else if (Response.bOk)
	{
		ResponseObj->SetField(TEXT("result"), MakeShareable(new FJsonValueObject(MakeShareable(new FJsonObject()))));
		ResponseObj->SetField(TEXT("error"), MakeShareable(new FJsonValueNull()));
	}
	else
	{
		ResponseObj->SetField(TEXT("result"), MakeShareable(new FJsonValueNull()));
		TSharedPtr<FJsonObject> ErrorObj = MakeShareable(new FJsonObject());
		ErrorObj->SetStringField(TEXT("code"), Response.ErrorCode);
		ErrorObj->SetStringField(TEXT("message"), Response.ErrorMessage);
		ResponseObj->SetField(TEXT("error"), MakeShareable(new FJsonValueObject(ErrorObj)));
	}

	FString ResponseStr;
	TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&ResponseStr);
	FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
	ResponseStr += TEXT("\n");

	FTCHARToUTF8 Converter(*ResponseStr);
	int32 Sent = 0;
	FScopeLock Lock(&ClientSocketCS);
	return Client->Send(reinterpret_cast<const uint8*>(Converter.Get()), Converter.Length(), Sent);
}
