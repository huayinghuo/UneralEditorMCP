// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "UMGASPlayerState.generated.h"

class UAbilitySystemComponent;

/** GAS 玩家状态：持有并管理 AbilitySystemComponent，跨 Pawn 生命周期保持 */
UCLASS()
class UNREALEDITORMCP_API AUMGASPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AUMGASPlayerState();

	/** 获取此玩家的 AbilitySystemComponent */
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

protected:
	/** ASC 在服务器和客户端都会复制 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
};
