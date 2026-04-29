#include "MCPBridgeDispatcher.h"

DEFINE_LOG_CATEGORY_STATIC(LogMCPDispatcher, Log, All);

void FMCPBridgeDispatcher::RegisterHandler(TSharedPtr<IMCPBridgeHandler> Handler)
{
	if (!Handler.IsValid()) return;
	const FString ActionName = Handler->GetActionName();
	Handlers.Add(ActionName, Handler);
}

bool FMCPBridgeDispatcher::Dispatch(
	const FString& Action,
	TSharedPtr<FJsonObject> Payload,
	TSharedPtr<FJsonObject>& OutResult,
	FString& OutErrorCode,
	FString& OutErrorMessage)
{
	TSharedPtr<IMCPBridgeHandler>* FoundHandler = Handlers.Find(Action);
	if (!FoundHandler || !(*FoundHandler).IsValid())
	{
		OutErrorCode = TEXT("UNKNOWN_ACTION");
		OutErrorMessage = FString::Printf(TEXT("Unknown action: %s"), *Action);
		UE_LOG(LogMCPDispatcher, Warning, TEXT("AUDIT: unknown action '%s'"), *Action);
		return false;
	}

	// Token 校验：仅当 Token 已配置时对 Write/Dangerous 操作执行
	if (!Token.IsEmpty())
	{
		EMCPActionCategory Cat = (*FoundHandler)->GetCategory();
		if (Cat == EMCPActionCategory::Write || Cat == EMCPActionCategory::Dangerous)
		{
			FString ClientToken;
			if (!Payload.IsValid() || !Payload->TryGetStringField(TEXT("_token"), ClientToken) || ClientToken != Token)
			{
				OutErrorCode = TEXT("TOKEN_REQUIRED");
				OutErrorMessage = TEXT("Valid _token required for write/dangerous operations");
				UE_LOG(LogMCPDispatcher, Warning, TEXT("AUDIT: token rejected for action '%s'"), *Action);
				return false;
			}
		}
	}

	bool bOk = (*FoundHandler)->Execute(Payload, OutResult, OutErrorCode, OutErrorMessage);

	// 审计日志：记录所有写/危险操作
	EMCPActionCategory Cat = (*FoundHandler)->GetCategory();
	if (Cat == EMCPActionCategory::Write || Cat == EMCPActionCategory::Dangerous)
	{
		UE_LOG(LogMCPDispatcher, Log, TEXT("AUDIT: action='%s' category=%d ok=%d error=%s"),
			*Action, static_cast<int32>(Cat), bOk, bOk ? TEXT("none") : *OutErrorCode);
	}

	return bOk;
}

EMCPActionCategory FMCPBridgeDispatcher::GetActionCategory(const FString& Action) const
{
	const TSharedPtr<IMCPBridgeHandler>* FoundHandler = Handlers.Find(Action);
	if (FoundHandler && (*FoundHandler).IsValid())
		return (*FoundHandler)->GetCategory();
	return EMCPActionCategory::Read;
}

TArray<TSharedPtr<FJsonValue>> FMCPBridgeDispatcher::GetAllActionsInfo() const
{
	TArray<TSharedPtr<FJsonValue>> Result;
	for (const TPair<FString, TSharedPtr<IMCPBridgeHandler>>& Pair : Handlers)
	{
		TSharedPtr<FJsonObject> Obj = MakeShareable(new FJsonObject());
		Obj->SetStringField(TEXT("action"), Pair.Key);
		Obj->SetNumberField(TEXT("category"), static_cast<int32>(Pair.Value->GetCategory()));
		Result.Add(MakeShareable(new FJsonValueObject(Obj)));
	}
	return Result;
}
