#include "Handlers/MCPTransactionHandlers.h"
#include "MCPBridgeHelpers.h"
#include "Editor.h"

DEFINE_LOG_CATEGORY_STATIC(LogMCPTransaction, Log, All);

// 事务状态机：同一时刻只允许一个活跃事务
static bool bTransactionActive = false;

// 供外部调用：断连等异常情况自动结束事务
void MCPTransaction_AutoEndIfActive()
{
	if (bTransactionActive && GEditor)
	{
		GEditor->EndTransaction();
		bTransactionActive = false;
		UE_LOG(LogMCPTransaction, Warning, TEXT("AUTO-END: Transaction auto-ended due to disconnect"));
	}
}

// ====== BeginTransaction ======
bool FMCPBeginTransactionHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	FString Description = TEXT("MCP Transaction");
	if (Payload.IsValid())
		Payload->TryGetStringField(TEXT("description"), Description);

	if (!GEditor)
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("NO_EDITOR"), TEXT("Editor not available"), OutErrorCode, OutErrorMessage);
		return false;
	}

	if (bTransactionActive)
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("TRANSACTION_ACTIVE"), TEXT("A transaction is already active. Call end_transaction first."), OutErrorCode, OutErrorMessage);
		return false;
	}

	GEditor->BeginTransaction(FText::FromString(Description));
	bTransactionActive = true;

	OutResult = MakeShareable(new FJsonObject());
	OutResult->SetStringField(TEXT("description"), Description);
	OutResult->SetBoolField(TEXT("started"), true);
	return true;
}

// ====== EndTransaction ======
bool FMCPEndTransactionHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	if (!GEditor)
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("NO_EDITOR"), TEXT("Editor not available"), OutErrorCode, OutErrorMessage);
		return false;
	}

	if (!bTransactionActive)
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("NO_ACTIVE_TRANSACTION"), TEXT("No active transaction to end."), OutErrorCode, OutErrorMessage);
		return false;
	}

	GEditor->EndTransaction();
	bTransactionActive = false;

	OutResult = MakeShareable(new FJsonObject());
	OutResult->SetBoolField(TEXT("ended"), true);
	return true;
}

// ====== Undo ======
bool FMCPUndoHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	if (!GEditor)
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("NO_EDITOR"), TEXT("Editor not available"), OutErrorCode, OutErrorMessage);
		return false;
	}

	// 有活跃事务时先自动结束
	if (bTransactionActive)
	{
		GEditor->EndTransaction();
		bTransactionActive = false;
		UE_LOG(LogMCPTransaction, Warning, TEXT("Auto-ended active transaction before undo"));
	}

	bool bSuccess = GEditor->UndoTransaction();

	OutResult = MakeShareable(new FJsonObject());
	OutResult->SetBoolField(TEXT("undone"), bSuccess);

	if (!bSuccess)
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("UNDO_FAILED"), TEXT("No transaction to undo or undo failed"), OutErrorCode, OutErrorMessage);
		return false;
	}
	return true;
}

// ====== Redo ======
bool FMCPRedoHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	if (!GEditor)
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("NO_EDITOR"), TEXT("Editor not available"), OutErrorCode, OutErrorMessage);
		return false;
	}

	// 有活跃事务时先自动结束
	if (bTransactionActive)
	{
		GEditor->EndTransaction();
		bTransactionActive = false;
		UE_LOG(LogMCPTransaction, Warning, TEXT("Auto-ended active transaction before redo"));
	}

	bool bSuccess = GEditor->RedoTransaction();

	OutResult = MakeShareable(new FJsonObject());
	OutResult->SetBoolField(TEXT("redone"), bSuccess);

	if (!bSuccess)
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("REDO_FAILED"), TEXT("No transaction to redo or redo failed"), OutErrorCode, OutErrorMessage);
		return false;
	}
	return true;
}
