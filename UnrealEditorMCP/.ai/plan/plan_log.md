# 计划变更日志

[2026-04-29] MVP 初始化 → 编译修复 → 联调（5 个运行时问题）
[2026-04-29] 阶段 6：执行层重构
[2026-04-29] Handler 拆分 + 只读/写命令
[2026-04-29] P0/P1：component + 资产 + 事务
[2026-04-29] 阶段 7：稳定性收敛（7 项修复）
[2026-04-29] 阶段 8：BP/Material/Screenshot + Widget/Dirty（10 Handler）
[2026-04-29] 阶段 9：安全权限体系（三级分类 + token + 审计）
[2026-04-29] 阶段 10 First Pass：分类修正 / ok 语义 / 事务状态机 / action registry
[2026-04-29] 阶段 11：深层资产编辑（Material 写入 + BP 变量/函数，5 Handler）
[2026-04-29] 阶段 10 Second Pass：失败语义 + 资产冲突 + 断连事务 + 配置增强
[2026-04-29] 阶段 10B 治理闭环：ini 配置 / 契约 registry / 队列丢弃 / 写语义 / BP 重名检测
[2026-04-30] 阶段 13 仓库治理：.gitignore / .editorconfig / .gitattributes / README.md
[2026-04-30] 阶段 11A：受控 Blueprint 图编辑（6 Handler + 2 辅助文件，37→43 Handler）
[2026-04-30] 阶段 11A 验收测试：编译通过 + 10/10 测试通过（tcp://127.0.0.1:9876）
[2026-04-30] 崩溃修复：Stop() 重入导致栈溢出（防重入守卫 + 编译验证）
[2026-04-30] 阶段 10C 规划：Python 自举配置收敛（BOOTSTRAP_MCP_CONFIG / online override / disconnect degradation / 11B 恢复顺序）
[2026-04-30] 阶段 10C 实现：3 文件修改 + 6/6 验收通过 + 11A 回归 10/10
[2026-04-30] 阶段 11B 实现：声明式 BP 图重建（apply_spec + export_spec, 2 Handler + 3 文件, 7/7 测试通过）
[2026-04-30] 修复任务逐项确认：16 项历史 Review Finding 全部核实（14 ✅ 已修复 / 2 ⚠️ 已知；3 项非阻塞待处理）
[2026-04-30] 10C/11B 最小修复包：7 项修复合入（arguments None / fallback 条件 / count 移除 / 前向声明 / export 严格返回 / CONNECTION_REJECTED / compiled 语义）
[2026-04-30] 外部 plan 规范化：标题层级统一，阶段编号/深度收敛
[2026-04-30] 10C/11B 复验通过：阻塞项全部修复，接受进入稳定基线
[2026-04-30] 阶段 12 启动：传输层与部署能力评估（评估矩阵 + 决策框架，非实现）
[2026-04-30] 仓库计划镜像：`plan_sync.md` 创建，外部 plan 为主，repo 内为精简协作镜像
[2026-04-30] 阶段 12A 实现：传输层稳态化与可观测性收口（8 文件变更，48→49 Handler，新增 get_bridge_runtime_status + last error 追踪 + 单客户端独占固定 + Python UEBridgeError 错误分类 + test_stage12a.ps1 验收脚本）
[2026-04-30] 阶段 14 实现：MCP Resources 知识层（4 static + 2 live resources，resources.py + test_stage14_resources.ps1，22/22 通过）
[2026-04-30] 阶段 14 v2：MCP 协议链路修复（AnyUrl→str 分发，ReadResourceContents 数据类，test_resources_mcp.py 真实 stdio 测试，27/27 通过）
[2026-04-30] 阶段 14A：资源契约统一 + 测试分层（runtime/status 字段一致化，Layer 1 TCP / Layer 2 MCP protocol 双模，27/27 通过）
[2026-04-30] 阶段 15A：BridgeClient 并发串行化（threading.Lock）+ README/plan_sync/plan_index/dev_last 收口
[2026-04-30] 阶段 16 Slice 1-4：Widget 能力完整深化（50→58 Handler，11 新增 + 2 增强，26/26 验收通过）
[2026-05-01] 阶段 17 命令式路径：7 Handler + helpers + 11B spec 扩展（58→65 Handler，编译通过）
[2026-05-01] 阶段 17 验收修复：apply_spec dispatch 补齐 3 节点类型 + README 同步 + test_stage17.ps1 创建（9/9 测试通过）
[2026-05-01] 阶段 18A：函数搜索+CDO默认值（3 Handler, 65→68, 编译通过）
[2026-05-01] 阶段 18B：PIE 运行时控制（5 Handler, 68→73, 编译通过）
[2026-05-02] 阶段 19：父类放宽 + CDO 通用属性 + GameplayTag + Python 输出（87→94 Handler，编译通过）
[2026-05-02] 20A：EI 资产创建防弹窗（三重检查 + static TSet）+ 超时警告机制（Server 线程 5s→TIMEOUT）
[2026-05-01] GAS 文档分工：`GASPlan/gas-game-plan.md` 写游戏项目计划，`.kilo/gas/gas-game-plan.md` 写 MCP 需求项
[2026-05-01] 阶段 A 完成：GAS 项目基线（.uproject 插件 + Build.cs 模块 + 6 类 12 文件 + 编译通过，20 actions 0 errors）
[2026-05-01] 阶段 B MCP 能力边界实测：GE 蓝图创建 + DurationPolicy ✓，Modifier/Tag/SetByCaller ✗→ 5 条 MCP 需求追加
