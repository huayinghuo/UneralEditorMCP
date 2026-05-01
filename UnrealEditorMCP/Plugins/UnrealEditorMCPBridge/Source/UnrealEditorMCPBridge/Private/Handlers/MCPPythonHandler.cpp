#include "Handlers/MCPPythonHandler.h"
#include "MCPBridgeHelpers.h"
#include "IPythonScriptPlugin.h"
#include "PythonScriptTypes.h"

bool FMCPExecutePythonHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	FString PythonCode;
	if (!Payload.IsValid() || !Payload->TryGetStringField(TEXT("code"), PythonCode))
	{ MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"), TEXT("payload.code is required"), OutErrorCode, OutErrorMessage); return false; }

	IPythonScriptPlugin* PythonPlugin = IPythonScriptPlugin::Get();
	if (!PythonPlugin) { MCPBridgeHelpers::BuildErrorResponse(TEXT("PYTHON_UNAVAILABLE"), TEXT("PythonScriptPlugin not enabled"), OutErrorCode, OutErrorMessage); return false; }

	FPythonCommandEx CmdEx;
	CmdEx.Command = PythonCode;
	CmdEx.ExecutionMode = EPythonCommandExecutionMode::ExecuteStatement;
	bool bSuccess = PythonPlugin->ExecPythonCommandEx(CmdEx);

	OutResult = MakeShareable(new FJsonObject());
	OutResult->SetBoolField(TEXT("success"), bSuccess);
	OutResult->SetStringField(TEXT("output"), CmdEx.CommandResult);
	if (!CmdEx.LogOutput.IsEmpty())
	{
		FString LogStr;
		for (const FPythonLogOutputEntry& Entry : CmdEx.LogOutput) { LogStr += Entry.Output + TEXT("\n"); }
		OutResult->SetStringField(TEXT("log"), LogStr);
	}

	if (!bSuccess)
	{ MCPBridgeHelpers::BuildErrorResponse(TEXT("PYTHON_EXEC_FAILED"), TEXT("Python execution failed"), OutErrorCode, OutErrorMessage); return false; }
	return true;
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
	OutResult->SetStringField(TEXT("output"), TEXT(""));
	return true;
}
