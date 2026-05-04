// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "UMGASPlayerController.generated.h"

/** GAS 玩家控制器：后续负责 Enhanced Input 绑定和 UI 创建 */
UCLASS()
class UNREALEDITORMCP_API AUMGASPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AUMGASPlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
};
