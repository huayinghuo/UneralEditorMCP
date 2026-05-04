# GAS 地牢战斗肉鸽学习与开发计划

> 本文件是当前 UEMCP 项目的 GAS 学习、游戏本体开发与联机同步验证计划。
> 主文档位置：`GASPlan/gas-game-plan.md`。
> MCP 能力需求项单独维护在 `.kilo/gas/gas-game-plan.md`。

## 0. 项目定位

### 0.1 游戏方向

目标是在 `UnrealEditorMCP` 项目本体中逐步实现一个地牢战斗肉鸽原型，风格参考《灰烬之国》一类俯视角/动作向地牢战斗体验。

核心体验包括：

- 每个角色拥有基础攻击模式与多段连击。
- 每个角色拥有多个可配置技能，技能可在局内遇到、装备、替换或升级。
- 玩家拥有多个装备槽，局内可获得、穿戴、分解装备。
- 局内存在金币、商店、随机事件与战斗奖励。
- 优先完成完整战斗系统，再扩展肉鸽构筑、装备与事件系统。
- 多人联机是核心学习目标，所有关键战斗逻辑都要按服务器权威、客户端预测和网络复制的思路设计。

### 0.2 技术方向

本计划以 Unreal Engine 5.3 的 Gameplay Ability System 为主线，逐步覆盖：

- AbilitySystemComponent 的挂载与初始化。
- Ability、Input、Activation、Commit、Cancel、Cooldown、Cost。
- AttributeSet 与属性复制。
- GameplayEffect 的即时、持续、周期、永久效果。
- GameplayTag 的状态、事件、免疫、触发与分类管理。
- GameplayCue 的表现层解耦。
- AbilityTask、Montage、Combo Window、TargetData。
- 网络复制、预测、回滚、Server RPC、RepNotify、NetUpdateFrequency。
- DataAsset/DataTable 驱动的技能、装备、怪物与房间配置。

### 0.3 项目边界

游戏功能实现位置：

- 项目本体模块：`UnrealEditorMCP/Source/UnrealEditorMCP/`
- 游戏内容资产：`UnrealEditorMCP/Content/`
- 配置文件：`UnrealEditorMCP/Config/`

MCP 插件位置：

- `UnrealEditorMCP/Plugins/UnrealEditorMCPBridge/`

边界约定：

- GAS 游戏代码优先放在项目本体模块，不放进 MCP 插件。
- MCP 插件只作为自动化辅助工具，不承载游戏运行时逻辑。
- 每个 GAS 阶段结束后，需要复盘当前 MCP 对 GAS/游戏开发的支持缺口。
- MCP 需求统一写入 `.kilo/gas/gas-game-plan.md`，作为后续 MCP 模块迭代依据。MCP 定位是通用引擎/蓝图编辑器，不对 GAS 做专项支持。

### 0.4 当前项目基线

当前项目本体模块非常轻量：

- `UnrealEditorMCP.Build.cs` 目前仅依赖 `Core`、`CoreUObject`、`Engine`、`InputCore`。
- `.uproject` 当前启用编辑器脚本、Python、建模工具插件，尚未启用 GameplayAbilities 插件。
- MCP Bridge 插件已有较完整编辑器自动化能力，但需要进一步验证对 GAS 资产和 C++ 游戏代码迭代的覆盖程度。

## 1. 总体阶段路线

| 阶段 | 目标 | 主要产物 | GAS 学习重点 | 联机重点 | MCP 复盘重点 |
|------|------|----------|--------------|----------|--------------|
| A | 项目 GAS 基线 | 启用 GAS 依赖、基础角色、ASC 初始化 | ASC、OwnerActor、AvatarActor | 服务器初始化、客户端同步 | 是否能自动改 Build.cs、uproject、创建 C++ 类 |
| B | 属性系统 | Health/Mana/Stamina/MoveSpeed 等属性 | AttributeSet、Pre/PostAttributeChange | 属性 RepNotify | 是否能创建属性类、默认值资产、测试属性变化 |
| C | 基础攻击与连击 | 普攻 Ability、多段连击、动画接口 | Ability 激活、Montage、AbilityTask | 输入预测、服务器确认 | 是否能创建 AnimMontage/节点/测试输入链路 |
| D | 技能系统 | 技能栏、冷却、消耗、技能升级 | GameplayEffect、Cooldown、Cost | 技能激活预测与失败回滚 | 是否能编辑 GE/GA 蓝图和 Tag |
| E | 伤害与战斗结算 | 伤害公式、击退、死亡、掉落 | ExecutionCalculation、Damage Meta Attribute | 服务器权威伤害 | 是否能批量生成测试战斗场景 |
| F | 装备系统 | 装备槽、词条、穿戴/分解 | GE 持久修饰、GrantedAbility | 装备状态复制 | 是否能生成 DataAsset/DataTable |
| G | 肉鸽局内构筑 | 奖励、商店、事件、技能升级 | Tag 驱动构筑条件 | 随机种子同步 | 是否能自动创建 UI/Widget/配置表 |
| H | 怪物与 AI | 怪物 ASC、技能、仇恨、刷怪 | AI 使用 Ability | AI 状态复制 | 是否能自动搭建测试关卡 |
| I | 多人同步专项 | 2 客户端验证、预测问题修复 | PredictionKey、TargetData | 延迟、丢包、Join In Progress | MCP 是否支持 PIE 多客户端控制 |
| J | 战斗闭环垂直切片 | 一间地牢完整战斗 | 全链路整合 | 权威、预测、复制闭环 | 输出 MCP 后续路线 |

## 2. 阶段 A：GAS 项目基线

### 2.1 学习目标

掌握 GAS 最小运行闭环：

- 为什么需要 `UAbilitySystemComponent`。
- ASC 放在 Pawn 还是 PlayerState 的取舍。
- `OwnerActor` 与 `AvatarActor` 的区别。
- `IAbilitySystemInterface` 的用途。
- 服务器与客户端分别在何时初始化 ASC。

本项目建议：

- 玩家 ASC 放在 `PlayerState`，便于死亡后换 Pawn、局内构筑和多人同步。
- 怪物 ASC 放在 Character 自身，简化 AI 生命周期。
- Character 作为 AvatarActor。

### 2.2 开发任务

1. 启用 GameplayAbilities 相关插件与模块依赖。
2. 在项目本体创建基础类：
   - `AUMGASPlayerState`
   - `AUMGASCharacterBase`
   - `AUMGASPlayerCharacter`
   - `AUMGASEnemyCharacter`
   - `AUMGASPlayerController`
   - `AUMGASGameMode`
3. 为 PlayerState 创建 ASC，并设置复制模式。
4. 在 Character 的 `PossessedBy` 和 `OnRep_PlayerState` 中初始化 ASC ActorInfo。
5. 创建一张最小测试地图或使用默认地图放置 PlayerStart。
6. 配置默认 Pawn、GameMode、PlayerController。

### 2.3 推荐代码原则

- 玩家 ASC：`SetIsReplicated(true)`，复制模式先用 `Mixed`。
- 怪物 ASC：可用 `Minimal` 或 `Mixed`，第一版先用 `Mixed` 方便调试。
- PlayerState 提高 `NetUpdateFrequency`，避免属性同步迟滞。
- 所有 GAS 类和公共方法补充中文注释，符合项目规范。

### 2.4 验收测试

- 编译通过。
- 单人 PIE 能正常生成玩家角色。
- Dedicated Server 或 Listen Server 下客户端能拥有有效 ASC。
- 打印或屏幕显示 ASC 初始化成功状态。
- 调用调试命令确认 PlayerState 与 Character 的 ActorInfo 已正确绑定。

### 2.5 阶段复盘问题

- MCP 是否能稳定创建 C++ 类并更新 Build.cs？
- MCP 是否能读取/修改 `.uproject` 插件列表？
- MCP 是否能设置项目默认 GameMode、Pawn、PlayerController？
- 当前 PIE 控制能力是否足以启动多人测试？

## 3. 阶段 B：AttributeSet 与基础属性

### 3.1 学习目标

理解属性系统的职责：

- BaseValue 与 CurrentValue。
- Meta Attribute 与持久属性。
- `PreAttributeChange`、`PostGameplayEffectExecute` 的区别。
- 属性复制和 `GAMEPLAYATTRIBUTE_REPNOTIFY`。
- Clamp 逻辑放在哪里。

### 3.2 属性设计

第一批属性：

- `Health`：当前生命。
- `MaxHealth`：最大生命。
- `Mana`：当前法力或能量。
- `MaxMana`：最大法力或能量。
- `Stamina`：体力，用于闪避、冲刺或重攻击。
- `MaxStamina`：最大体力。
- `AttackPower`：基础攻击力。
- `AbilityPower`：技能强度。
- `Armor`：护甲。
- `MoveSpeed`：移动速度。
- `Damage`：伤害 Meta Attribute，仅用于 GE 结算，不复制为 UI 长期状态。

### 3.3 开发任务

1. 创建 `UUMCombatAttributeSet`。
2. 为属性添加复制声明和 OnRep 函数。
3. 实现属性 Clamp：生命、法力、体力不得超过最大值，不得小于 0。
4. 实现 `Damage` 结算：从 Health 扣减并清零 Damage。
5. 创建初始化属性 GameplayEffect：`GE_Hero_AttributeInit`。
6. 在角色 ASC 初始化后应用初始属性 GE。
7. 创建最小 UI 或调试文本显示 Health/Mana/Stamina。

### 3.4 验收测试

- 服务端应用初始化 GE 后，客户端能看到正确属性。
- 修改 MaxHealth 后 Health Clamp 正确。
- 应用 Damage 后 Health 正确减少。
- 客户端不能直接伪造最终 Health，伤害由服务器权威执行。

### 3.5 阶段复盘问题

- MCP 是否能创建 AttributeSet C++ 模板？
- MCP 是否能自动生成 GE 蓝图资产？
- MCP 是否能设置 GE 的 Modifier？
- MCP 是否能验证属性复制状态？

## 4. 阶段 C：基础攻击、多段连击与输入

### 4.1 学习目标

理解 Ability 如何表达动作：

- Ability 激活条件。
- 输入绑定到 AbilitySpec。
- `CommitAbility` 的时机。
- `PlayMontageAndWait` 与动画事件。
- Combo Window、输入缓冲、取消窗口。
- 客户端预测激活与服务器确认。

### 4.2 战斗模型

基础攻击分为：

- 轻攻击 1、2、3 段。
- 重攻击或蓄力攻击。
- 闪避或位移技能。
- 命中检测第一版可用碰撞盒或短距离 Sweep。
- 后续升级到 GameplayTargeting 或自定义 TargetData。

### 4.3 开发任务

1. 引入 Enhanced Input 到项目本体模块依赖。
2. 创建输入动作：移动、视角/朝向、轻攻击、重攻击、闪避、技能 1-4、交互。
3. 创建 `UUMGameplayAbilityBase`。
4. 创建 `UUMGA_MeleeCombo`。
5. 创建 Combo 状态 Tags：
   - `State.Attacking`
   - `State.Combo.WindowOpen`
   - `State.Combo.BufferedInput`
   - `Ability.Attack.Light`
   - `Ability.Attack.Heavy`
6. 在 Ability 中播放 Montage 并根据 AnimNotify 打开/关闭连击窗口。
7. 设计命中事件：动画命中帧触发服务器权威命中检测。
8. 命中后应用伤害 GE。

### 4.4 验收测试

- 单人环境下可以执行 3 段轻攻击。
- 连击窗口外输入不会错误进入下一段。
- 客户端按键后动作响应及时，服务器拒绝时能恢复。
- 多客户端下，一个玩家攻击另一个玩家，伤害只由服务器结算。
- 攻击状态 Tag 能正确阻止闪避或被闪避取消，规则按设计执行。

### 4.5 阶段复盘问题

- MCP 是否能创建 Enhanced Input 资产并绑定默认 Mapping Context？
- MCP 是否能创建 Montage 或至少配置动画资产引用？
- MCP 是否能在蓝图图表中创建 Ability 激活链路？
- MCP 是否能启动并控制多客户端 PIE 自动测试攻击同步？

## 5. 阶段 D：技能、冷却、消耗与局内升级

### 5.1 学习目标

掌握技能系统的 GAS 表达方式：

- AbilitySpec 与赋予技能。
- Ability Level。
- Cooldown GE 与 Cost GE。
- `CanActivateAbility` 与失败 Tag。
- 技能升级如何影响 GE 参数、伤害、范围、冷却。
- 技能槽如何与输入绑定解耦。

### 5.2 技能设计

第一批技能：

- `GA_DashStrike`：突进斩，位移并造成伤害。
- `GA_FireNova`：范围爆发。
- `GA_IceShard`：直线投射物或射线。
- `GA_ShieldGuard`：短时护盾或减伤。

技能槽：

- 主动技能槽 1-4。
- 被动技能槽若干。
- 技能可由局内奖励授予。
- 技能升级修改 Ability Level 或替换 DataAsset 参数。

### 5.3 开发任务

1. 创建技能数据资产类型 `UUMAbilityDataAsset`。
2. 定义技能 ID、显示名、图标、AbilityClass、初始等级、最大等级、标签、冷却、消耗。
3. 实现技能授予与移除接口。
4. 实现技能槽系统：槽位绑定 AbilitySpecHandle。
5. 创建 Cost GE 与 Cooldown GE。
6. 创建技能升级接口：提升 Ability Level 并刷新 UI。
7. 设计技能选择奖励：局内战斗后随机 3 选 1。

### 5.4 验收测试

- 技能可以被授予、装备到槽位、通过输入释放。
- 冷却期间不能再次释放，并能通过 Tag/UI 观察。
- 法力/能量不足时激活失败。
- 技能升级后伤害或冷却变化可验证。
- 多客户端下技能释放表现、伤害、冷却一致。

### 5.5 阶段复盘问题

- MCP 是否能批量创建 GA/GE/DataAsset？
- MCP 是否能编辑 GameplayTag 配置文件？
- MCP 是否能读取 AbilitySpec/ActiveGameplayEffect 运行时状态？
- MCP 是否能生成技能奖励测试数据？

## 6. 阶段 E：伤害、状态、死亡与奖励闭环

### 6.1 学习目标

深入理解战斗结算：

- Damage 作为 Meta Attribute 的好处。
- ExecutionCalculation 与 MMC 的差异。
- SetByCaller Magnitude。
- 免疫、格挡、暴击、易伤等规则如何用 Tag 表达。
- 死亡状态如何阻止输入和技能。

### 6.2 开发任务

1. 创建 `UUMDamageExecutionCalculation`。
2. 设计伤害上下文：来源、目标、技能 Tag、伤害类型、暴击、击退。
3. 创建伤害类型 Tags：
   - `Damage.Physical`
   - `Damage.Fire`
   - `Damage.Ice`
   - `Damage.Poison`
4. 创建状态 Tags：
   - `State.Dead`
   - `State.Stunned`
   - `State.Invulnerable`
   - `State.Blocking`
5. 实现死亡流程：停止 Ability、禁用输入、播放死亡表现、掉落奖励。
6. 实现金币与经验或局内成长资源。
7. 创建战斗房间清怪奖励流程。

### 6.3 验收测试

- 不同伤害类型可被护甲或抗性影响。
- 死亡后不能继续释放技能。
- 击杀奖励只由服务器发放一次。
- 多人协作击杀时奖励归属规则明确。

### 6.4 阶段复盘问题

- MCP 是否能构造自动战斗测试场景？
- MCP 是否能读取 Tag 堆栈、Active GE、属性变化历史？
- MCP 是否能生成伤害矩阵测试用例？

## 7. 阶段 F-J：肉鸽系统与联机专项

### 7.1 阶段 F：装备系统

开发内容：

- 装备槽：武器、头、胸、手、腿、饰品 1、饰品 2。
- 装备品质：普通、稀有、史诗、传说。
- 装备词条：固定词条、随机词条、套装 Tag。
- 装备效果：通过持久 GE 修改属性，通过 GrantedAbility 授予技能。
- 分解装备：返还金币、材料或升级资源。

验收重点：

- 穿戴装备后属性立即变化并复制。
- 卸下装备后对应 GE 被移除。
- 装备授予的 Ability 被正确移除，不遗留输入绑定。

### 7.2 阶段 G：局内随机事件与商店

开发内容：

- 金币系统。
- 战斗后 3 选 1 奖励。
- 商店随机出售技能、装备、恢复、事件入口。
- 随机事件：花金币换奖励、牺牲生命换技能、诅咒换高品质装备。
- 局内随机使用服务器种子，客户端只接收结果。

验收重点：

- 多客户端看到一致商店和奖励选项。
- 购买和选择奖励由服务器确认。
- 随机事件不能被客户端伪造。

### 7.3 阶段 H：怪物与 AI

开发内容：

- 怪物基础 AttributeSet。
- 怪物技能 Ability。
- 近战、远程、精英、Boss 原型。
- AI 感知、追击、攻击、撤退。
- 刷怪器和房间波次。

验收重点：

- AI 在服务器运行，客户端只同步结果。
- 怪物技能伤害与表现一致。
- Boss 技能可用 Tag 阻止或阶段切换。

### 7.4 阶段 I：多人同步与预测专项

专项验证：

- Listen Server 与 Dedicated Server 差异。
- 2 客户端和 4 客户端战斗同步。
- 输入延迟下普攻和技能手感。
- 预测失败回滚是否可接受。
- Join In Progress 是否能同步当前属性、装备、技能、冷却。
- 网络模拟：延迟、抖动、丢包。

关键调试：

- `showdebug abilitysystem`。
- ASC 复制模式观察。
- PredictionKey 生命周期。
- GameplayCue 是否重复播放或漏播。

### 7.5 阶段 J：战斗系统垂直切片

最终切片目标：

- 1 个可选玩家角色。
- 1 套三段普攻。
- 4 个主动技能。
- 6-10 件装备。
- 3 类普通怪、1 个精英怪、1 个 Boss。
- 1 个地牢房间战斗流程。
- 战斗后奖励、商店或随机事件至少实现一种。
- 2 客户端联机完整通关一间房。

验收标准：

- 客户端输入响应不明显卡顿。
- 所有伤害、死亡、掉落、购买、奖励选择均服务器权威。
- UI 显示的生命、技能冷却、装备属性与服务器状态一致。
- MCP 需求池已整理为后续开发优先级列表。

## 8. MCP 能力需求池

> 格式：`[日期] [阶段] 需求 | 当前不足 | 优先级 | 验收标准`

| 日期 | 阶段 | 需求 | 当前不足 | 优先级 | 验收标准 |
|------|------|------|----------|--------|----------|
| 2026-05-01 | A | 支持启用 `.uproject` 插件与编辑 Build.cs 模块依赖的安全工作流 | 当前 MCP 以 UE 编辑器资产/蓝图操作为主，尚未沉淀 GAS 项目初始化模板 | P0 | 能自动检查并提示 GameplayAbilities、GameplayTags、GameplayTasks、EnhancedInput 依赖是否启用 |
| 2026-05-01 | A | 支持创建项目本体 C++ GAS 类模板 | 当前 Handler 偏向资产和蓝图编辑，缺少面向 GAS 的类生成模板 | P0 | 能生成 ASC PlayerState、CharacterBase、AttributeSet、GameplayAbilityBase 的规范代码骨架 |
| 2026-05-01 | B | 支持 GameplayEffect 资产创建与 Modifier 配置 | 现有能力未确认能精确创建 GE 并设置 Attribute Modifier、Duration、Period、Stacking | P0 | 能创建 Init/Damage/Cooldown/Cost GE 并读取校验配置 |
| 2026-05-01 | C | 支持多客户端 PIE 自动化控制与运行时状态采集 | 当前已有 PIE start/stop/is_running，但多人客户端、网络模拟和 ASC 调试读取能力需验证/增强 | P0 | 能启动 2 客户端 PIE，触发输入/命令，读取双方属性、Tag、Active GE |
| 2026-05-01 | C | 支持 Enhanced Input 到 GAS Ability 的端到端配置检查 | 已有 Enhanced Input Handler，但缺少 GAS AbilitySpec/InputID 绑定专项验证 | P1 | 能检查输入动作、Mapping Context、角色绑定和 Ability 激活链路 |
| 2026-05-01 | D | 支持 GameplayTag 配置文件读写与 Tag 查询 | GAS 大量依赖 Tag，当前缺少专门的 Tag 管理需求文档 | P0 | 能新增/查询/校验 Tag，避免重复和命名不一致 |
| 2026-05-01 | D | 支持 Ability/Effect/DataAsset 批量生成 | 技能和装备将产生大量重复资产，手工创建效率低 | P1 | 能根据 JSON/Spec 批量创建 GA、GE、技能数据资产和基础测试用例 |
| 2026-05-01 | E | 支持 ASC 运行时诊断工具 | 当前运行时状态更多集中在 Actor/PIE，缺少 ASC 内部状态读取 | P0 | 能读取指定 Actor 的属性、Owned Tags、Blocked Tags、Active GE、Granted Abilities |
| 2026-05-01 | F | 支持 DataAsset/DataTable 结构化编辑 | 装备、技能、怪物、奖励表需要大量数据驱动 | P1 | 能创建、更新、导出 DataAsset/DataTable，并做字段校验 |
| 2026-05-01 | I | 支持网络模拟与同步验收脚本 | GAS 学习重点包含预测和复制，需要可重复的延迟/丢包测试 | P1 | 能设置网络延迟/丢包，运行固定战斗脚本并输出差异报告 |

## 9. 每阶段执行模板

每个阶段按以下固定节奏执行：

1. 学习：阅读并总结本阶段 GAS 概念。
2. 设计：确认本项目采用的类结构、数据结构、Tag 命名和网络权威边界。
3. 实现：只在项目本体或 Content 中落地游戏功能，不修改 MCP 插件，除非阶段目标明确是 MCP 增强。
4. 编译：运行 UE 5.3 UnrealBuildTool 编译项目。
5. 运行：启动 UE Editor 或 PIE 做最小验证。
6. 联机：阶段 C 之后每阶段至少做一次 2 客户端验证。
7. 复盘：记录 GAS 学习结论、实现结果、缺陷、MCP 能力需求。
8. 更新：持续更新本文件的进度、需求池和下一步计划。

## 10. 当前进度

| 日期 | 状态 | 内容 |
|------|------|------|
| 2026-05-01 | 已创建 | 建立 GAS 学习与开发主计划，明确地牢战斗肉鸽方向、项目边界、阶段路线 |
| 2026-05-01 | ✅ 完成 | **阶段 A：GAS 项目基线** — 启用插件、修改 Build.cs、12 文件 6 类创建、编译通过（20 actions, 0 errors） |
| 2026-05-04 | ✅ MCP 完成 | **阶段 B 步骤 5-7：MCP 创建 GE + Tag** — GE_Hero_AttributeInit（10 Modifier）+ GE_Damage_Test（SetByCaller）+ Data.Damage Tag 全部通过 MCP 创建/配置 |

### 阶段 B 进度细分

| 步骤 | 内容 | 状态 | 备注 |
|------|------|------|------|
| 1 | 创建 UMCombatAttributeSet.h | ✅ 用户已完成 | 编译通过 |
| 2 | 创建 UMCombatAttributeSet.cpp | ✅ 用户已完成 | 编译通过 |
| 3 | PlayerCharacter 注册 AttributeSet | ✅ 用户已完成 | 在 InitAbilityActorInfo 中 |
| 4 | 编译验证 | ✅ 用户已完成 | — |
| 5 | 创建 GE_Hero_AttributeInit（10 Modifier） | ✅ MCP 创建 | Python 创建蓝图 + `blueprint_add_cdo_array` ×10 |
| 6 | 创建 GameplayTag `Data.Damage` | ✅ MCP 创建 | `create_gameplay_tag` |
| 7 | 创建 GE_Damage_Test（SetByCaller） | ✅ MCP 创建 | Python 创建 + `blueprint_add_cdo_array` |
| 8 | 步骤 6 代码（加载应用初始 GE） | 🔲 待用户 | `UMGASPlayerCharacter.cpp` |
| 9 | PIE 验证 | 🔲 待用户 | `showdebug abilitysystem` |

### 阶段 A 交付物

| 文件 | 说明 |
|------|------|
| `Source/UnrealEditorMCP/GAS/UMGASPlayerState.h/.cpp` | ASC 放在 PlayerState，Mixed 复制模式，NetUpdateFrequency=100 |
| `Source/UnrealEditorMCP/GAS/UMGASCharacterBase.h/.cpp` | 角色基类，IAbilitySystemInterface 统一入口 |
| `Source/UnrealEditorMCP/GAS/UMGASPlayerCharacter.h/.cpp` | PossessedBy/OnRep_PlayerState 双路径绑定 ASC ActorInfo |
| `Source/UnrealEditorMCP/GAS/UMGASEnemyCharacter.h/.cpp` | 怪物自身持有 ASC，自包含模式 |
| `Source/UnrealEditorMCP/GAS/UMGASPlayerController.h/.cpp` | 玩家控制器骨架 |
| `Source/UnrealEditorMCP/GAS/UMGASGameMode.h/.cpp` | 设置默认 Pawn/PlayerState/Controller |
| `UnrealEditorMCP.uproject` | 启用 GameplayAbilities、EnhancedInput 插件 |
| `UnrealEditorMCP.Build.cs` | 添加 4 个 GAS 模块依赖 |

### 阶段 A 踩坑记录

- `GameplayTags` 和 `GameplayTasks` 在 `Build.cs` 中是模块名（可写），但在 `.uproject` 中不是独立插件（UE 5.3 内置于引擎，不能作为插件启用）。
- 编译时需要关闭 UE Editor 否则 DLL 被锁。
- AGameMode（非 AGameModeBase）支持 DefaultPawnClass、PlayerStateClass、PlayerControllerClass。

## 11. 下一步执行清单

用户待完成：

1. **步骤 6 代码**：在 `UMGASPlayerCharacter.cpp` 的 `InitAbilityActorInfo()` 末尾添加：
   ```cpp
   if (HasAuthority()) {
       UClass* InitGEClass = LoadClass<UGameplayEffect>(nullptr,
           TEXT("/Game/GAS/GameplayEffects/GE_Hero_AttributeInit.GE_Hero_AttributeInit_C"));
       if (InitGEClass && ASC) {
           FGameplayEffectContextHandle Ctx = ASC->MakeEffectContext();
           FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(InitGEClass, 1.0f, Ctx);
           if (Spec.IsValid()) ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
       }
   }
   ```
2. **PIE 验证**：`showdebug abilitysystem` → 确认 Health=500 等属性生效
3. **Editor 检查**：打开两个 GE 蓝图确认 Modifier 配置（尤其 Attribute 选择器是否因 ImportText 丢失引用）
