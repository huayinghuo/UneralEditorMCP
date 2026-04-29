#include "Handlers/MCPViewportHandlers.h"
#include "MCPBridgeHelpers.h"
#include "Editor.h"
#include "LevelEditorViewport.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"

DEFINE_LOG_CATEGORY_STATIC(LogMCPViewport, Log, All);

bool FMCPViewportScreenshotHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	FString Filename;
	if (Payload.IsValid() && Payload->TryGetStringField(TEXT("filename"), Filename))
	{
		// 使用客户端提供的文件名
	}
	else
	{
		FDateTime Now = FDateTime::Now();
		Filename = FString::Printf(TEXT("mcp_screenshot_%04d%02d%02d_%02d%02d%02d.png"),
			Now.GetYear(), Now.GetMonth(), Now.GetDay(),
			Now.GetHour(), Now.GetMinute(), Now.GetSecond());
	}

	FString OutputDir = FPaths::ProjectSavedDir() / TEXT("Screenshots");
	IFileManager::Get().MakeDirectory(*OutputDir, true);
	FString FilePath = OutputDir / Filename;

	// 获取当前活跃视口
	FViewport* Viewport = GEditor->GetActiveViewport();
	if (!Viewport)
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("NO_VIEWPORT"), TEXT("No active viewport available"), OutErrorCode, OutErrorMessage);
		return false;
	}

	// 读取视口像素
	TArray<FColor> Bitmap;
	FIntPoint SizePoint = Viewport->GetSizeXY();
	int32 Width = SizePoint.X;
	int32 Height = SizePoint.Y;
	if (!Viewport->ReadPixels(Bitmap) || Bitmap.Num() == 0)
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("READ_FAILED"), TEXT("Failed to read viewport pixels"), OutErrorCode, OutErrorMessage);
		return false;
	}

	// 保存为 PNG
	FImageView ImageView(Bitmap.GetData(), Width, Height, ERawImageFormat::BGRA8);
	bool bSaved = FImageUtils::SaveImageAutoFormat(*FilePath, ImageView);

	OutResult = MakeShareable(new FJsonObject());
	OutResult->SetStringField(TEXT("file_path"), FilePath);
	OutResult->SetNumberField(TEXT("width"), Width);
	OutResult->SetNumberField(TEXT("height"), Height);
	OutResult->SetBoolField(TEXT("saved"), bSaved);

	if (!bSaved)
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("SAVE_FAILED"), TEXT("Failed to save screenshot"), OutErrorCode, OutErrorMessage);
		return false;
	}

	return true;
}
