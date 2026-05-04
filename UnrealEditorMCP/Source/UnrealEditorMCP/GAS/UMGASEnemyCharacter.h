// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UMGASCharacterBase.h"
#include "UMGASEnemyCharacter.generated.h"

class UAbilitySystemComponent;

/** 怪物角色：自身持有 ASC，初始化在 BeginPlay 完成，便于 AI 生命周期 */
UCLASS()
class UNREALEDITORMCP_API AUMGASEnemyCharacter : public AUMGASCharacterBase
{
	GENERATED_BODY()

public:
	AUMGASEnemyCharacter();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	virtual void InitAbilityActorInfo() override;

protected:
	virtual void BeginPlay() override;

	/** 怪物 ASC 放在自身，服务器和客户端都会复制 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
};
