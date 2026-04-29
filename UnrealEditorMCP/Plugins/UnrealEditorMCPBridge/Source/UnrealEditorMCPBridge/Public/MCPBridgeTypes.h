#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

// 网络线程解析 JSON 后入队的待处理请求
struct FMCPPendingRequest
{
	FString Id;                      // 请求唯一标识，原样回传
	FString Action;                  // 目标动作名称（ping / get_editor_info 等）
	TSharedPtr<FJsonObject> Payload; // 请求携带的 JSON 参数体
};

// Game Thread 处理完成后入队的待发送响应
struct FMCPPendingResponse
{
	FString Id;                          // 对应请求的 Id
	bool bOk;                            // 处理是否成功（true 读 result，false 读 error）
	TSharedPtr<FJsonObject> ResultObj;   // 成功时的返回数据
	FString ErrorCode;                   // 失败时的错误码（如 UNKNOWN_ACTION）
	FString ErrorMessage;                // 失败时的错误描述
};
