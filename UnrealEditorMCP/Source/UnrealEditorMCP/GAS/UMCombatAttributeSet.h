// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "UMCombatAttributeSet.generated.h"


// 属性访问器宏：GAS 的 GE 和 ExecCalc 查询属性时使用这个宏生成的静态函数
// ATTRIBUTE_ACCESSORS(UClassName, PropertyName) 展开为 Get/Set/Init 三个静态函数
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)
/**
 * 战斗属性集：管理角色的生命、法力、体力、攻击力、护甲和伤害通道
 * 包含 10 个持久属性 + 1 个 Meta 属性（Damage）
 */
UCLASS()
class UNREALEDITORMCP_API UUMCombatAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UUMCombatAttributeSet();

	// ========== 生命 ==========
	/** 当前生命值，受伤害后由 PostGameplayEffectExecute 扣减 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Attributes" , ReplicatedUsing = OnRep_Health)
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(UUMCombatAttributeSet, Health)

	/** 最大生命值，由CE初始化，装备可加成 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Attributes" , ReplicatedUsing = OnRep_MaxHealth)
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UUMCombatAttributeSet, MaxHealth)

	// ========== 法力/能量 ==========
  UPROPERTY(BlueprintReadOnly, Category = "Combat|Mana", ReplicatedUsing = OnRep_Mana)
	FGameplayAttributeData Mana;
	ATTRIBUTE_ACCESSORS(UUMCombatAttributeSet, Mana)

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Mana", ReplicatedUsing = OnRep_MaxMana)
	FGameplayAttributeData MaxMana;
	ATTRIBUTE_ACCESSORS(UUMCombatAttributeSet, MaxMana)

	// ========== 体力（闪避/冲刺/重攻击消耗） ==========
	UPROPERTY(BlueprintReadOnly, Category = "Combat|Stamina", ReplicatedUsing = OnRep_Stamina)
	FGameplayAttributeData Stamina;
	ATTRIBUTE_ACCESSORS(UUMCombatAttributeSet, Stamina)

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Stamina", ReplicatedUsing = OnRep_MaxStamina)
	FGameplayAttributeData MaxStamina;
	ATTRIBUTE_ACCESSORS(UUMCombatAttributeSet, MaxStamina)

	// ========== 战斗属性 ==========
	/** 基础攻击力：普攻和物理技能的基础伤害来源 */
	UPROPERTY(BlueprintReadOnly, Category = "Combat|Offense", ReplicatedUsing = OnRep_AttackPower)
	FGameplayAttributeData AttackPower;
	ATTRIBUTE_ACCESSORS(UUMCombatAttributeSet, AttackPower)

	/** 技能强度：法术类技能的伤害系数 */
	UPROPERTY(BlueprintReadOnly, Category = "Combat|Offense", ReplicatedUsing = OnRep_AbilityPower)
	FGameplayAttributeData AbilityPower;
	ATTRIBUTE_ACCESSORS(UUMCombatAttributeSet, AbilityPower)

	// ========== 防御 ==========
	UPROPERTY(BlueprintReadOnly, Category = "Combat|Defense", ReplicatedUsing = OnRep_Armor)
	FGameplayAttributeData Armor;
	ATTRIBUTE_ACCESSORS(UUMCombatAttributeSet, Armor)

	// ========== 移动 ==========
	UPROPERTY(BlueprintReadOnly, Category = "Combat|Movement", ReplicatedUsing = OnRep_MoveSpeed)
	FGameplayAttributeData MoveSpeed;
	ATTRIBUTE_ACCESSORS(UUMCombatAttributeSet, MoveSpeed)

	// ========== Meta 属性（不复制） ==========
	/**
	 * 伤害瞬态通道：GE 写入此值 → PostGameplayEffectExecute 扣减 Health 后清零
	 * 不复制，不作为持久 UI 状态
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Combat|Damage")
	FGameplayAttributeData Damage;
	ATTRIBUTE_ACCESSORS(UUMCombatAttributeSet, Damage)


	// ========== 重写虚函数 ==========
	/** 属性修改前调用：Clamp 生命/法力/体力到 [0, Max] */
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;

	/** GE 执行完成后调用（仅服务器）：消费 Damage、扣减 Health */
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	/** 控制哪些属性需要网络复制 */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;


protected:
	/** 用 Clamp 约束值到 [MinValue, MaxValue] */
	void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue, const FGameplayAttribute& MaxAttribute) const;

	// ========== OnRep 函数（RepNotify） ==========
	UFUNCTION()
	virtual void OnRep_Health(const FGameplayAttributeData& OldHealth);
	UFUNCTION()
	virtual void OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth);
	UFUNCTION()
	virtual void OnRep_Mana(const FGameplayAttributeData& OldMana);
	UFUNCTION()
	virtual void OnRep_MaxMana(const FGameplayAttributeData& OldMaxMana);
	UFUNCTION()
	virtual void OnRep_Stamina(const FGameplayAttributeData& OldStamina);
	UFUNCTION()
	virtual void OnRep_MaxStamina(const FGameplayAttributeData& OldMaxStamina);
	UFUNCTION()
	virtual void OnRep_AttackPower(const FGameplayAttributeData& OldAttackPower);
	UFUNCTION()
	virtual void OnRep_AbilityPower(const FGameplayAttributeData& OldAbilityPower);
	UFUNCTION()
	virtual void OnRep_Armor(const FGameplayAttributeData& OldArmor);
	UFUNCTION()
	virtual void OnRep_MoveSpeed(const FGameplayAttributeData& OldMoveSpeed);
};
