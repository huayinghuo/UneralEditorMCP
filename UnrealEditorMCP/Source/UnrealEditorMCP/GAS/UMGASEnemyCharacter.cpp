// Copyright Epic Games, Inc. All Rights Reserved.

#include "UMGASEnemyCharacter.h"
#include "AbilitySystemComponent.h"

AUMGASEnemyCharacter::AUMGASEnemyCharacter()
{
	// 怪物 ASC 放在自身：AI 生命周期简单，不需要跨 Pawn 保持
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);

	// Mixed 模式便于调试：技能 Tag 和 Cue 对所有客户端可见
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
}

UAbilitySystemComponent* AUMGASEnemyCharacter::GetAbilitySystemComponent() const
{
	// 怪物直接返回自身 ASC，不委托给外部
	return AbilitySystemComponent;
}

void AUMGASEnemyCharacter::InitAbilityActorInfo()
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	// 怪物既是 OwnerActor 也是 AvatarActor（自包含模式）
	AbilitySystemComponent->InitAbilityActorInfo(this, this);


}

void AUMGASEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	// 怪物在 BeginPlay 时初始化 ActorInfo（服务器端）
	// 注意：InitAbilityActorInfo 内部会处理 Authority 判断
	InitAbilityActorInfo();
}
