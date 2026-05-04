#include "Handlers/MCPSystemHandlers.h"
#include "MCPBridgeHelpers.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"

DEFINE_LOG_CATEGORY_STATIC(LogMCPSystem, Log, All);

bool FMCPCloseModalWindowHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	if (!FSlateApplication::IsInitialized())
	{ MCPBridgeHelpers::BuildErrorResponse(TEXT("SLATE_UNAVAILABLE"), TEXT("Slate not initialized"), OutErrorCode, OutErrorMessage); return false; }

	TSharedPtr<SWindow> Modal = FSlateApplication::Get().GetActiveModalWindow();
	if (!Modal.IsValid())
	{
		OutResult = MakeShareable(new FJsonObject());
		OutResult->SetBoolField(TEXT("dismissed"), false);
		OutResult->SetStringField(TEXT("message"), TEXT("No active modal window"));
		return true;
	}

	FString Title = Modal->GetTitle().ToString();
	FSlateApplication::Get().RequestDestroyWindow(Modal.ToSharedRef());

	OutResult = MakeShareable(new FJsonObject());
	OutResult->SetBoolField(TEXT("dismissed"), true);
	OutResult->SetStringField(TEXT("title"), Title);
	return true;
}
