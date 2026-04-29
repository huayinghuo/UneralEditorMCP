#include "Handlers/MCPPythonHandler.h"
#include "MCPBridgeHelpers.h"
#include "IPythonScriptPlugin.h"

bool FMCPExecutePythonHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	FString PythonCode;
	if (!Payload.IsValid() || !Payload->TryGetStringField(TEXT("code"), PythonCode))
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"), TEXT("payload.code is required"), OutErrorCode, OutErrorMessage);
		return false;
	}

	IPythonScriptPlugin* PythonPlugin = IPythonScriptPlugin::Get();
	if (!PythonPlugin)
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("PYTHON_UNAVAILABLE"), TEXT("PythonScriptPlugin is not available. Ensure Python Editor Script Plugin is enabled."), OutErrorCode, OutErrorMessage);
		return false;
	}

	bool bSuccess = PythonPlugin->ExecPythonCommand(*PythonCode);

	OutResult = MakeShareable(new FJsonObject());
	OutResult->SetBoolField(TEXT("success"), bSuccess);

	if (!bSuccess)
	{
		// 严格顶层语义：执行失败 → ok=false + 错误码
		OutResult->SetStringField(TEXT("error"), TEXT("Python execution failed"));
		MCPBridgeHelpers::BuildErrorResponse(TEXT("PYTHON_EXEC_FAILED"), TEXT("Python execution failed"), OutErrorCode, OutErrorMessage);
		return false;
	}

	OutResult->SetStringField(TEXT("output"), TEXT(""));
	return true;
}
