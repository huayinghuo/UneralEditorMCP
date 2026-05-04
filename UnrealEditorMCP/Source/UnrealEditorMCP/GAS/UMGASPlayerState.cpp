// Copyright Epic Games, Inc. All Rights Reserved.

#include "UMGASPlayerState.h"
#include "AbilitySystemComponent.h"

AUMGASPlayerState::AUMGASPlayerState()
{
	// 创建 ASC 子对象，确保在服务器和客户端都存在
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));

	// 核心：使 ASC 参与网络复制
	AbilitySystemComponent->SetIsReplicated(true);

	// Mixed 模式：GameplayEffect 仅复制给拥有者客户端，Tags 和 Cues 复制给所有客户端
	// 这是多人游戏推荐模式，在性能和表现同步间取得平衡
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	// 玩家状态默认 NetUpdateFrequency 为 1 Hz，提升以支持属性快速同步
	NetUpdateFrequency = 100.0f;
}

UAbilitySystemComponent* AUMGASPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}
