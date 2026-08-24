# WorldSimDemo 可直接执行 Backlog V1

## 1. 文档用途

`master_implementation_plan_v1.md` 决定方向，`implementation_cards_*.md` 解释设计，本文件决定下一次具体改什么。执行者不需要重新选择架构，只能按本文件顺序领取第一个 Ready 任务。

## 2. 单任务完成规则

每张任务票据必须在一次连续修改中完成，状态流转为 `TODO -> DOING -> CODED -> VERIFIED`。

`CODED` 只代表源码已写；只有满足票据中的 Verify 才能标记 `VERIFIED`。依赖检查只接受 `VERIFIED`，但在没有 Windows UE 5.8 时，明确标注 `PRECOMPILE` 的任务允许依赖上游 `CODED` 继续准备。

执行者必须遵守：

1. 只修改 `Files` 列出的文件。
2. 遇到需要修改范围外文件时停止该票，先新增依赖票据。
3. 不顺手重构、不增加计划外枚举、不创建第二套时间/区域/旅行类型。
4. 每票结束更新状态、实际文件、偏差和验收证据。
5. Verify 失败时保持 `CODED`，不得标记完成。

## 3. 阶段门

| Gate | 必须满足 | 解锁范围 |
|---|---|---|
| G0A Precompile Ready | **READY**：E001-E013 为 CODED，E014-E015 为 VERIFIED，静态契约通过 | Windows 首次编译 |
| G0B UE Build Green | E020-E023 为 VERIFIED | M1 一致性改造 |
| G1 Consistency Green | E101-E114 为 VERIFIED，Demo 审计 0 Error | M2 生活切片 |
| G2 Two-Day Slice | M2 全卡 VERIFIED | M3 社会系统 |
| G3 Local Society | M3 全卡 VERIFIED | M4 长期规模 |
| G4 Long Run | 一年压测、存档和 LOD VERIFIED | M5-M7 |
| G5 Planet Travel | M5 全卡 VERIFIED | M6 星际层 |
| G6 Interstellar | M6 全卡 VERIFIED | M8 文明危机 |
| G7 Gameplay Slice | M7 全卡 VERIFIED | 内容生产 |
| G8 Crisis Continuation | M8 全卡 VERIFIED | M9 宇宙生产化 |

## 4. 当前执行批次：M0 Precompile

### E001 查询公共类型

Status：`CODED`。Mode：`PRECOMPILE`。Card：`M0-IC02`。

Depends：无。

Files：`Source/WorldSimDemo/WorldSimTypes.h`。

Do：

1. 在 `FActiveWorldActivity` 后新增 `FWorldEventQuery` 和 `FSimulationDebugLimits`。
2. `FWorldEventQuery` 严格使用 IC02 中列出的八个字段。
3. 为两个结构增加 `Normalize()` 普通 C++ 方法；Clamp 上限固定 1000。
4. 时间范围非法时 `IsValid()` 返回 false，不在结构内部输出日志。
5. 不新建 `WorldSimQueryTypes.h`，避免首编译前增加 generated header 风险。

Output：两个 BlueprintType 结构和纯 C++ 规范化方法。

Verify：静态检查字段名唯一、generated include 仍为最后一个 include；UE Verify 留到 E020。

Stop：若 UHT 不允许结构中的普通方法，记录错误后再改为 BlueprintFunctionLibrary，禁止预判删除。

Actual：已在 `WorldSimTypes.h` 增加两个结构、Normalize 和 IsValid；静态字段及 include 顺序检查通过。UE 5.8 UHT 验证等待 E020。

### E002 TruthLedger 有界查询

Status：`CODED`。Mode：`PRECOMPILE`。Card：`M0-IC03`。

Depends：E001 `CODED`。

Files：`TruthLedgerSubsystem.h/.cpp`。

Do：

1. 新增 `QueryEvents`、`GetRecentEvents`、`TryGetEvent` 声明与实现。
2. `RecordEvent` 增加 EventType 非 None 校验和 EventId 去重。
3. 实现单遍过滤、稳定排序和最大数量截断。
4. 排序同分钟时使用 `EventId.ToString(EGuidFormats::Digits)`。
5. 不建立索引，索引属于 M4。

Verify：构造 6 条事件的 UE 测试留 E022；本票静态确认声明/实现签名一致。

Actual：已实现三个查询 API、空 EventType 拒绝、EventId 去重、稳定排序和有界截断；静态签名检查通过，行为验证等待 E022。

### E003 Person 查询

Status：`CODED`。Mode：`PRECOMPILE`。Card：`M0-IC04`。

Depends：E001 `CODED`。

Files：`PersonSubsystem.h/.cpp`。

Do：新增 `GetPeople(int32 MaxResults)`；Clamp 1..1000；从 Map 复制；按 PersonId Digits 排序；截断。不得返回内部 Map。

Verify：空表返回空；相同表连续调用顺序相同。

Actual：已增加有界 BlueprintPure 查询，按 PersonId Digits 稳定排序并截断；UE 行为验证等待 E022。

### E004 Activity 查询

Status：`CODED`。Mode：`PRECOMPILE`。Card：`M0-IC04`。

Depends：E001 `CODED`。

Files：`WorldSimulationSubsystem.h/.cpp`。

Do：新增 `GetActiveActivities(int32 MaxResults)`；按 RemainingMinutes 升序，同值按 PersonId；不调用 EvaluateDecision。

Verify：查询前后 ActiveActivities 数量、活动内容和世界时间不变。

Actual：已增加有界 BlueprintPure 查询，按 RemainingMinutes 和 PersonId 稳定排序；实现不调用决策或时间 API，UE 行为验证等待 E022。

### E005 Commitment 查询

Status：`CODED`。Mode：`PRECOMPILE`。Card：`M0-IC04`。

Depends：E001 `CODED`。

Files：`CommitmentSubsystem.h/.cpp`。

Do：新增全部和 Active 查询；Active 排除 Completed、Cancelled、Failed；按 PlannedStart、CommitmentId；现有 `GetCommitmentsForSubject` 改为 BlueprintPure，但不改变行为。

Verify：终态不出现在 Active；查询无状态变化。

Actual：已增加全部/Active 有界查询，按 PlannedStart 和 CommitmentId 稳定排序；人物承诺查询已标记 BlueprintPure，UE 行为验证等待 E022。

### E006 Opportunity 事实查询

Status：`CODED`。Mode：`PRECOMPILE`。Card：`M0-IC04`。

Depends：E001 `CODED`。

Files：`OpportunityCompilerSubsystem.h/.cpp`。

Do：新增 `GetAvailableOpportunities(At, RegionId, MaxResults) const`；只用 IsAvailableAt 和 Region 过滤；按 ExpiresAt 升序、Urgency 降序、ID；绝不调用 Motivation。

Verify：调用前后 LastDecision 不新增、AvailableUses 不减少。

Actual：已增加有界 BlueprintPure 事实查询，按过期时间、紧迫度和 ID 稳定排序；实现不访问 Motivation，不修改 AvailableUses，UE 行为验证等待 E022。

### E007 Knowledge 查询

Status：`CODED`。Mode：`PRECOMPILE`。Card：`M0-IC04`。

Depends：E001 `CODED`。

Files：`KnowledgeSubsystem.h/.cpp`。

Do：新增 `TryGetMessage` 和 `GetKnowledgeRecords`；按 LearnedAt 降序、MessageId；限制数量；KnowerId 无效返回空。

Verify：不存在 MessageId 返回 false 且不改 Out；查询不创建 KnowledgeRecord。

Actual：已增加消息按 ID 查询和人物知识记录有界查询；记录按 LearnedAt 降序及 MessageId 稳定排序，查询不写 KnowledgeRecords，UE 行为验证等待 E022。

### E008 Bootstrap 类型和空服务

Status：`CODED`。Mode：`PRECOMPILE`。Card：`M0-IC05`。

Depends：E003-E007 `CODED`。

Files：新建 `WorldSimDemoBootstrapSubsystem.h/.cpp`，扩展 `WorldSimTypes.h`。

Do：新增 `FDemoWorldIds`；创建 UWorldSubsystem；声明 `CreateDemoWorld`、`IsDemoWorldCreated`；本票只完成依赖获取、重复创建拒绝、失败原因，不写数据。

Verify：依赖缺失返回 false；第二次调用逻辑路径存在；无 Reset API。

Actual：已增加 FDemoWorldIds、WorldSubsystem 骨架、依赖检查和 AlreadyCreated 分支；数据写入明确返回 DataCreationNotImplemented，等待 E009-E011，未增加 Reset API。

### E009 稳定 Demo GUID

Status：`CODED`。Mode：`PRECOMPILE`。Card：`M0-IC05`。

Depends：E008 `CODED`。

Files：`WorldSimDemoBootstrapSubsystem.h/.cpp`。

Do：实现私有 `MakeStableGuid(Seed, Label)`；使用 UE MD5/SHA 可用 API 将 Seed+ASCII Label 映射 16 字节；禁止 `FGuid::NewGuid()`；为三个角色和两个承诺定义常量标签。

Verify：同 Seed/Label 相同，不同 Label 不同；全零结果重新哈希带后缀。

Actual：已使用 FMD5 对 UTF-8 `Seed:Label` 做稳定 128 位映射，按显式字节序构造 GUID；定义三个角色和两个承诺标签，全零结果使用固定后缀重新哈希。UE 确定性测试等待 E022。

### E010 Demo 人物和区域

Status：`CODED`。Mode：`PRECOMPILE`。Card：`M0-IC05`。

Depends：E009 `CODED`。

Files：`WorldSimDemoBootstrapSubsystem.cpp`。

Do：按 IC05 固定数据创建 RegionSnapshot、玩家、Worker、Traveller 和因果状态；检查每个返回 ID；Player `bAutonomous=false`；三人 CurrentRegion=`Port_Aster`。

Verify：任一步失败返回 `Step:Reason`；只有全部成功才继续机会阶段。

Actual：已创建 Port_Aster 快照、固定 GUID 的玩家/Worker/Traveller 及三套因果状态；玩家关闭自治，NPC 配置不同驱动力。人物和状态失败均返回 Step:Reason，成功后明确停在 E011。

### E011 Demo 机会和承诺

Status：`CODED`。Mode：`PRECOMPILE`。Card：`M0-IC05`。

Depends：E010 `CODED`。

Files：`WorldSimDemoBootstrapSubsystem.cpp`。

Do：注册五类固定机会；公共服务 AvailableUses=-1；创建 Traveller 安全承诺和 Worker 风险承诺；OutIds 填满后设置 bCreated。

Verify：所有 ID 有效；第二次 Create 返回 AlreadyCreated；同 Seed ID 一致。

Actual：已注册稳定 ID 的餐食、班次、休息、诊疗和危险检查机会，并创建 Traveller 安全转运与 Worker 风险任务承诺；所有返回 ID 必须匹配后才设置 bCreated，成功时清空失败原因。

### E012 Debug Snapshot 类型

Status：`CODED`。Mode：`PRECOMPILE`。Card：`M0-IC06`。

Depends：E001 `CODED`。

Files：新建 `WorldSimDebugTypes.h`。

Do：定义 `FPersonDebugSnapshot`、`FWorldDebugSnapshot`；generated include 最后；只含值类型；字段严格按 IC06；不放计算方法。

Verify：无 UObject 指针、无嵌套不受 UHT 支持容器。

Actual：已新增 FPersonDebugSnapshot 和 FWorldDebugSnapshot，全部使用值类型与一层 TArray；当前活动标识明确使用 ActiveOpportunityId，等待 E102 引入规范 ActivityId 后迁移。

### E013 Debug Snapshot 服务

Status：`CODED`。Mode：`PRECOMPILE`。Card：`M0-IC06`。

Depends：E002-E007、E012 `CODED`。

Files：新建 `WorldSimDebugSubsystem.h/.cpp`。

Do：实现 `BuildSnapshot`；一次获取各数组并建立临时 Map/MultiMap；人物循环只查 Map；读取 LastDecision 不重新 Evaluate；稳定排序。

Verify：连续构建快照内容相同；事件数、活动数、世界时间不变。

Actual：已增加 WorldSubsystem 调试聚合器；一次获取有界数组并建立 Activity Map 与 Commitment MultiMap，人物循环仅读取 CausalState、LastDecision 和 KnowledgeRecord，不触发决策或时间推进。

### E014 静态契约脚本

Status：`VERIFIED`。Mode：`PRECOMPILE`。

Depends：E001-E013 `CODED`。

Files：新建 `scripts/check_source_contracts.sh`。

Do：检查 generated include、头实现 API 名、旧错误字段、EngineAssociation 5.8、Target Latest、禁止新增 GameInstanceSubsystem；只做可证明的文本契约，不声称编译。

Verify：脚本退出 0；人为放入旧字段样本时退出非 0，随后撤销样本。

Actual：已增加仓库静态契约脚本，覆盖 generated include、UE 5.8/Latest 配置、关键 API 声明实现、旧错误字段和 GameInstanceSubsystem 禁令，并用临时样本自测旧字段检测器。

### E015 M0 Precompile 文档状态

Status：`VERIFIED`。Mode：`PRECOMPILE`。

Depends：E014 `VERIFIED`。

Files：本文件、`master_implementation_plan_v1.md`。

Do：记录 E001-E014 实际改动和偏差；G0A 标记 Ready；不得把 M0 标为 DONE。

Verify：没有 TODO 的前置任务被误标 VERIFIED。

Actual：E001-E013 均保持 CODED 并等待 UE，E014 静态脚本已真实退出 0；G0A 已标记 READY，M0 仍未完成，下一票为受 Windows 环境约束的 E020。

## 5. Windows UE 批次

### E019 Windows 构建入口

Status：`VERIFIED`。Mode：`PRECOMPILE-SUPPORT`。

Depends：G0A。

Files：`scripts/build_ue58_windows.ps1`。

Do：检查 UE 5.8、项目关联、优先 Visual Studio 2026 18.0+（回退兼容 Visual Studio 2022 17.14+）及 C++ 游戏工作负载；生成项目文件；构建 `WorldSimDemoEditor Win64 Development`；将日志写入 `Saved/BuildLogs`；任一步失败返回非零退出码。

Run：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build_ue58_windows.ps1
```

若 UE 安装到其他目录：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build_ue58_windows.ps1 -EngineRoot "D:\Epic Games\UE_5.8"
```

Verify：Windows 11、UE 5.8.1、Visual Studio 2026 实测完成工程生成、Editor 构建和日志保存，退出成功。

### E020 UE 项目生成和首次构建

Status：`VERIFIED`。Card：`M0-IC01`。

Depends：G0A、E019 脚本已复制到 Windows 工作区。

Files：不预设；先只生成工程和构建。

Do：记录 UE/VS 版本；清理生成目录；生成项目；构建 Development Editor；保存第一份完整错误日志。

Verify：Windows 实测 UHT 与 Development Editor 构建退出 0；UE 5.8.1 编辑器成功加载 `WorldSimDemo` 模块。

### E021 编译错误逐组修复

Status：`VERIFIED`。

Depends：E020 已产生日志。

Do：每轮只修第一个根因组；记录 Error、Cause、Files、Fix；重新完整构建；直到 0。不得同时开发新功能。

Verify：Development Editor 构建成功。

Observed error 001：Epic Launcher 的 UE 5.8.1 安装中不存在 `Engine/Build/BatchFiles/GenerateProjectFiles.bat`，旧 E019 脚本在进入 Build.bat 前错误停止。

Fix 001：生成项目文件改为三级策略：存在 GenerateProjectFiles.bat 时使用；否则使用 `Engine/Binaries/DotNET/UnrealBuildTool/UnrealBuildTool.exe -ProjectFiles`；两者都不存在时给出警告并继续 Build.bat。工程文件生成不是命令行 Editor 构建的前置条件。

Observed error 002：UE 5.8 UHT 拒绝 `TMap<FGuid, TArray<FDailyScheduleEntry>>`，嵌套容器即使不是 UPROPERTY 也不能作为 UCLASS 成员。

Fix 002：新增 `FDailySchedule` USTRUCT 包装数组，内部缓存改为 `TMap<FGuid, FDailySchedule>`；Windows 完整重建成功。

Verify evidence：UE 5.8.1 日志显示 Engine initialized、地图检查 0 Error/0 Warning、`WorldSimDemoGameMode` 加载且 PIE 成功启动。

### E022 C++ Automation Smoke Tests

Status：`CODED-WAITING-WINDOWS`。

Depends：E021 `VERIFIED`。

Files：新建 `WorldSimDemoTests.cpp`，必要时 Build.cs 增加开发测试依赖。

Do：实现 Truth 6 事件查询、稳定 Demo GUID、查询纯度、Bootstrap 重复拒绝四组测试；使用 Editor Automation Framework。

Verify：命令行运行 `WorldSim.M0.*` 全绿。

Actual：新增四组独立临时 UWorld 自动化测试，覆盖 6 条 Truth 查询与重复 ID、同 Seed Bootstrap ID、查询纯度和重复初始化拒绝；等待 Windows 命令行执行验证。

Observed test error 001：四个测试均被发现，但首个测试创建默认名 `Untitled_1` 的临时世界时与编辑器已有 Level/WorldSettings 重名并触发 Fatal。

Fix 001：测试夹具为每个临时世界分配 `WorldSimAutomation_N` 进程内唯一名称，避免 Transient WorldSettings 命名冲突；等待 Windows 复测。

Observed test error 002：唯一 UWorld 名称已生效，但 UE 仍把测试世界放入共享 `/Temp/Untitled_1` 包，PersistentLevel 内的 WorldSettings 继续重名。

Fix 002：为每个测试世界同时创建唯一 `/Temp/WorldSimAutomation_N` UPackage，并显式传给 `UWorld::CreateWorld`，隔离 World、Level 和 WorldSettings 命名空间；等待 Windows 复测。

### E023 Blueprint Smoke Assets

Status：`BLOCKED-WINDOWS`。Card：`M0-IC07`。

Depends：E022 `VERIFIED`。

Do：在 UE 创建 IC07 指定 Map、Widget、Controller；连接初始化、三个推进按钮和快照刷新。

Verify：PIE 初始化、推进、重复初始化、重开 PIE 均符合断言；保存截图和 Output Log 摘要。

## 6. M1 原子执行顺序

| Ticket | Card | 实施内容 | Depends | Files | Verify |
|---|---|---|---|---|---|
| E101 | M1-IC01 | FWorldTime 增加差值、比较和仅调度器可用 SetCurrentTime | G0B | WorldSimTypes, WorldTimeSubsystem | 时间不倒退，负推进拒绝 |
| E102 | M1-IC02 | 活动增加 ActivityId/StartedAt/ExpectedEnd/State | E101 | WorldSimTypes, WorldSimulationSubsystem | 旧活动创建字段完整 |
| E103 | M1-IC02 | CompleteActivity 幂等提取 | E102 | WorldSimulationSubsystem | 重复完成只有一条事实 |
| E104 | M1-IC02 | InterruptActivity 和策略表 | E103 | WorldSimulationSubsystem | 中断后不再自然完成 |
| E105 | M1-IC01 | Commitment 下一关键时间查询 | E101 | CommitmentSubsystem | 返回严格晚于 From 的最近点 |
| E106 | M1-IC01 | Activity/Opportunity 下一关键时间查询 | E102 | WorldSimulation, OpportunityCompiler | 无关键点返回 false |
| E107 | M1-IC01 | 关键点推进循环 | E103,E105,E106 | WorldSimulation, WorldTime | 分步/整段推进一致 |
| E108 | M1-IC03 | Presence ReplaceFuturePresence | E101 | PresenceSubsystem | 失败原子回滚、区间无重叠 |
| E109 | M1-IC03 | Commitment 到 Presence 映射 | E107,E108 | Commitment, Presence | 出发/到达边界唯一 |
| E110 | M1-IC04 | Life outcome 关闭活动与未来承诺 | E104,E109 | Commitment, WorldSimulation, Presence | 死者无活动和 Present |
| E111 | M1-IC05 | Truth IdempotencyKey | E002,E107 | WorldSimTypes, TruthLedger | 重复 key 返回原 EventId |
| E112 | M1-IC06 | Message 发布时间、渠道和 AvailableAt | E111 | WorldSimTypes, Knowledge | 到达前不可学习 |
| E113 | M1-IC07 | Consistency AuditPerson | E108-E112 | 新建 Consistency files | 注入十类错误均捕获 |
| E114 | M1-IC07 | M1 回归矩阵 | E113 | Tests, Demo | 审计 0 Error，推进签名一致 |

每张 E101-E114 的字段、算法和断言以 `implementation_cards_m0_m1_v1.md` 对应 IC 为唯一实现说明。表格顺序不可跳过。

## 7. M2-M9 可领取顺序

| Order | Card | Depends | Gate result |
|---:|---|---|---|
| 201 | M2-IC01 身份创建 | G1 | Player identity DTO/API |
| 202 | M2-IC02 家庭住宅出生 | 201 | Causal spawn |
| 203 | M2-IC03 服务提供者 | 202 | Food/bed/medical supply |
| 204 | M2-IC05 工作合同 | 202 | Shift commitments |
| 205 | M2-IC06 选择解释 | 203,204 | Score breakdown |
| 206 | M2-IC04 详细生理 | 203 | Hot physiology |
| 207 | M2-IC07 两日验收 | 201-206 | G2 |
| 301 | M3-IC01 关系图 | G2 | Sparse relations |
| 302 | M3-IC02 组织权限 | 301 | Organization decisions |
| 303 | M3-IC03 感知证据 | 301 | Observations/evidence |
| 304 | M3-IC04 报警调度 | 302,303 | Real response travel |
| 305 | M3-IC05 案件调查 | 303,304 | G3 |
| 401 | M4-IC01 Simulation LOD | G3 | Tier transition |
| 402 | M4-IC02 区域宏观 | 401 | Aggregate regions |
| 403 | M4-IC03 原子存档 | 401,402 | Save/load |
| 404 | M4-IC04 历史归档 | 403 | Bounded history |
| 405 | M4-IC05 压测 | 401-404 | G4 |
| 501 | M5-IC01 交通图 | G4 | Route plans |
| 502 | M5-IC02 票务事务 | 501 | Capacity-safe tickets |
| 503 | M5-IC03 旅行 Presence | 502 | Vehicle regions |
| 504 | M5-IC04 异常旅行 | 503 | G5 |
| 601 | M6-IC01 天体位置 | G5 | Hierarchical locations |
| 602 | M6-IC02 曲率旅行 | 601 | Warp commitments |
| 603 | M6-IC03 通信路由 | 601,503 | Delayed messages |
| 604 | M6-IC04 贸易自治 | 602,603 | G6 |
| 701 | M7-IC01 修炼数据 | G4 | Canonical progression |
| 702 | M7-IC02 系统建议 | 701,M1-IC06 | Knowledge-safe advice |
| 703 | M7-IC03 感源动作 | 701 | Playable abilities |
| 704 | M7-IC04 遗迹事件图 | 702,703 | G7 |
| 801 | M8-IC01 组织战略 | G6,M3-IC02 | Strategic actions |
| 802 | M8-IC02 Day 0-120 | 801,704 | Crisis timeline |
| 803 | M8-IC03 战争延续 | 802 | G8 |
| 901 | M9-IC01 宇宙生成 | G4 | Deterministic universe |
| 902 | M9-IC02 历史生成 | 901,801 | Causal histories |
| 903 | M9-IC03 层级坐标 | 601,901 | Spatial frames |
| 904 | M9-IC04 Streaming | 903,401 | Presentation loading |
| 905 | M9-IC05 细节动作 | 904,206,703 | Production detail |

## 8. 变更控制

发现计划错误时，不直接在代码中绕过。新增 `PLAN-CHANGE-xxx` 记录：原任务、发现证据、影响任务、推荐修改、是否破坏存档。只有更新本 Backlog 和对应实施卡后才能继续。

## 9. 下一条唯一可执行任务

`E022 C++ Automation Smoke Tests`。

E022 源码已完成，下一步必须在 Windows 运行 `WorldSim.M0.*`；全绿后转到 E023 Blueprint Smoke Assets。

## 10. 计划变更记录

### PLAN-CHANGE-001 FWorldEventQuery 字段数量笔误

原任务写“九个字段”，但 `M0-IC02` 实际只定义 SubjectId、RegionId、EventType、FromInclusive、ToExclusive、bUseTimeRange、MaxResults、bNewestFirst 八个字段。以明确列出的八字段契约为准，不增加无需求来源的字段。影响范围仅为 E001 文案，不改变后续任务依赖。

### PLAN-CHANGE-002 M0 调试快照尚无 ActivityId

M0-IC06 要求 ActiveActivityId，但当前 FActiveWorldActivity 只有 OpportunityId，规范 ActivityId 计划在 E102 增加。M0 快照暂使用名称准确的 ActiveOpportunityId，禁止用 OpportunityId 冒充 ActivityId；E102 增加 ActivityId 时同步迁移快照字段。影响 E012-E013 和 E102，不改变阶段顺序。

### PLAN-CHANGE-003 增加 Windows 一键构建入口

G0A 到 E020 原计划要求手工检查环境、生成项目和构建，容易丢失完整错误日志。新增 E019 PowerShell 入口，只自动执行 E020 已要求的动作，不改变阶段门、不替代真实 UE 编译，也不允许在 Mac 上标记成功。

### PLAN-CHANGE-004 UE 5.8 改用推荐 Visual Studio 2026

Epic 的 UE 5.8 官方矩阵已将 Visual Studio 2026 18.0+列为普通开发推荐版本，同时支持 Visual Studio 2022 17.14+。原计划沿用了旧版 UE 的 VS 2022 建议，现已修正：E019 优先检测 VS 2026，缺失时才回退 VS 2022；推荐 MSVC 14.50 和 Windows SDK 10.0.26100。

### PLAN-CHANGE-005 兼容 Epic Launcher 二进制引擎生成方式

Windows 实测证明 UE 5.8.1 Launcher 安装不保证包含源码版的 GenerateProjectFiles.bat。E019 改为优先 BatchFiles 脚本、回退 UnrealBuildTool `-ProjectFiles`、最后允许跳过解决方案生成直接执行 Build.bat。此修改不降低真实编译门槛。
