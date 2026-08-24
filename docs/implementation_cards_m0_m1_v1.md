# M0-M1 代码级实施卡 V1

## 0. 实施卡使用方法

每张卡必须包含九项：目标、前置、修改文件、数据结构、公开 API、算法、错误处理、验收场景、完成证据。实施时不得只完成 API 壳；算法和验收断言缺一不可。

统一代码约束：

1. 世界模拟服务继续使用 `UWorldSubsystem`。
2. 规范时间继续使用 `FWorldTime.Minute`，不混用现实 `FDateTime`。
3. 区域继续使用 `FName`，不得引入第二套整数区域 ID。
4. Blueprint 只能通过函数访问数据，不返回内部容器引用。
5. 查询默认有数量上限，所有返回数组使用稳定排序。
6. 规范状态修改必须产生事实或明确标记为纯缓存变化。
7. 随机结果使用世界种子、实体 ID 和事件时间生成确定性种子。

# M0 编译与可观测基线

## M0-IC01 UE 5.8 首次编译修复循环

对应工作包：`M0-WP01`、`M0-WP02`。

### 目标

让当前源码通过 UE 5.8 的 UHT、C++ 编译和编辑器模块加载。此卡不新增玩法。

### 前置

Windows、UE 5.8 最新补丁、推荐 Visual Studio 2026 18.0+（最低兼容 Visual Studio 2022 17.14+）、推荐 MSVC 14.50、推荐 Windows SDK 10.0.26100。

### 修改范围

- `WorldSimDemo.uproject`
- `Source/WorldSimDemo.Target.cs`
- `Source/WorldSimDemoEditor.Target.cs`
- `Source/WorldSimDemo/WorldSimDemo.Build.cs`
- 仅修改编译错误指向的 `.h/.cpp`

### 执行算法

1. 删除本地旧 `Binaries`、`Intermediate`、`.vs`，不删除 `Content` 和源码。
2. 用 UE 5.8 生成项目文件。
3. 首先构建 `WorldSimDemoEditor Win64 Development`。
4. 若 UHT 失败，只修复第一组同根错误，例如 generated include、反射不支持类型、重复声明。
5. 若 C++ 失败，只修复第一组同根错误，例如字段名、include、const 不匹配。
6. 若链接失败，检查模块依赖、实现缺失和 API 导出。
7. 若模块加载失败，读取 UE 日志中的第一个项目栈错误。
8. 每轮修复后重新完整构建，不在错误未清零时新增系统。

### 错误处理规则

- 不通过把类移出 UHT 或删除 Blueprint 暴露来掩盖设计错误。
- `BuildSettingsVersion.Latest` 若被 UE 5.8 明确拒绝，则采用该安装版本生成项目的默认值。
- 版本 API 变化必须记录在迁移说明，不能用未解释的条件编译堆叠。

### 验收断言

1. UHT 完成且无项目反射错误。
2. Editor Target 构建退出码为 0。
3. UE 编辑器打开空关卡且模块已加载。
4. Output Log 不包含 `WorldSimDemo` 类加载失败。

### 完成证据

保存 UE 完整版本、VS 工具链版本、构建命令、退出码和首个成功日志摘要。

## M0-IC02 通用有界查询类型

对应工作包：`M0-WP03`。

### 目标

给调试 UI 和后续存档提供统一查询方式，避免为每个界面复制遍历逻辑。

### 修改文件

- `WorldSimTypes.h`
- 新建 `WorldSimQueryTypes.h`，若 UHT 编译复杂则暂时合并进 `WorldSimTypes.h`

### 新增数据结构

`FWorldEventQuery`：

- `FGuid SubjectId`，无效表示不过滤
- `FName RegionId`，None 表示不过滤
- `FName EventType`，None 表示不过滤
- `FWorldTime FromInclusive`
- `FWorldTime ToExclusive`
- `bool bUseTimeRange`
- `int32 MaxResults = 100`
- `bool bNewestFirst = true`

`FSimulationDebugLimits`：

- `MaxPeople = 100`
- `MaxActivities = 100`
- `MaxCommitments = 100`
- `MaxEvents = 200`
- `MaxMessages = 200`

### 规范化函数

新增非 UFUNCTION 帮助函数 `Normalize()`：把所有最大数量夹到 `1..1000`；启用时间范围时要求 `FromInclusive < ToExclusive`，否则查询返回空并输出失败原因。

### 验收断言

1. `MaxResults=-1` 不会导致无限复制。
2. 空过滤器按稳定顺序返回最近数据。
3. 反向时间范围返回空，不崩溃。

## M0-IC03 TruthLedger 查询实现

对应工作包：`M0-WP03`。

### 修改文件

- `TruthLedgerSubsystem.h`
- `TruthLedgerSubsystem.cpp`

### 新增 API

```cpp
UFUNCTION(BlueprintPure)
TArray<FWorldEvent> QueryEvents(const FWorldEventQuery& Query) const;

UFUNCTION(BlueprintPure)
TArray<FWorldEvent> GetRecentEvents(int32 MaxResults = 100) const;

UFUNCTION(BlueprintPure)
bool TryGetEvent(FGuid EventId, FWorldEvent& OutEvent) const;
```

### 具体算法

1. `RecordEvent` 为无 ID 事件生成 ID。
2. 写入前检查同 ID；已经存在则拒绝第二次写入并返回空 GUID，不能覆盖历史。
3. `QueryEvents` 单次遍历 `Events`。
4. 依次应用主体、区域、类型、时间范围过滤。
5. 按 `OccurredAt.Minute` 排序；同一分钟按 `EventId` 字符串或 GUID 稳定比较。
6. 根据 `bNewestFirst` 决定方向。
7. 截断到规范化后的 `MaxResults`。
8. M4 再替换为索引，当前先保证正确性。

### 错误处理

- 无效 EventId 查询返回 false。
- 重复 EventId 写入不改变原事件。
- 空 Summary 可以记录系统事件，但 EventType 不允许 None。

### 验收场景

创建 6 条跨两个主体、两个区域、三个时间点的事件，断言：主体过滤 3 条、区域过滤 3 条、时间过滤 2 条、最大数量 1 条、重复 ID 后总数不变。

## M0-IC04 人物、活动、承诺和机会查询

对应工作包：`M0-WP03`。

### 修改文件与 API

`PersonSubsystem.h/.cpp`：

```cpp
TArray<FPersonLite> GetPeople(int32 MaxResults = 100) const;
```

按 `PersonId` 稳定排序并截断。

`WorldSimulationSubsystem.h/.cpp`：

```cpp
TArray<FActiveWorldActivity> GetActiveActivities(int32 MaxResults = 100) const;
```

按剩余分钟、人物 ID 排序。

`CommitmentSubsystem.h/.cpp`：

```cpp
TArray<FCommitment> GetCommitments(int32 MaxResults = 100) const;
TArray<FCommitment> GetActiveCommitments(int32 MaxResults = 100) const;
```

Active 排除 `Completed/Cancelled/Failed`，按开始时间和 ID 排序。

`OpportunityCompilerSubsystem.h/.cpp`：

```cpp
TArray<FWorldOpportunity> GetAvailableOpportunities(FWorldTime At, FName RegionId, int32 MaxResults = 100) const;
```

不调用动机评估，只查询事实可用性；按过期时间、紧迫度和 ID 排序。

`KnowledgeSubsystem.h/.cpp`：

```cpp
bool TryGetMessage(FGuid MessageId, FWorldMessage& OutMessage) const;
TArray<FMessageKnowledge> GetKnowledgeRecords(FGuid KnowerId, int32 MaxResults = 100) const;
```

### 验收断言

1. 查询不改变任何系统状态。
2. 相同状态连续查询返回相同顺序。
3. 超限请求最多返回 1000 条。
4. Active 查询不返回终态承诺。

## M0-IC05 Demo Bootstrap 数据契约

对应工作包：`M0-WP04`。

### 新建文件

- `WorldSimDemoBootstrapSubsystem.h`
- `WorldSimDemoBootstrapSubsystem.cpp`

### 新增结构

`FDemoWorldIds`：`RegionId`、`PlayerId`、`WorkerId`、`TravellerId`、`SafeCommitmentId`、`RiskCommitmentId`、主要 OpportunityId 数组。

### 新增 API

```cpp
UFUNCTION(BlueprintCallable)
bool CreateDemoWorld(int32 Seed, FDemoWorldIds& OutIds, FString& OutFailureReason);

UFUNCTION(BlueprintPure)
bool IsDemoWorldCreated() const;
```

暂不提供 Reset，因为现有子系统没有安全清空协议。需要重置时重开 PIE 世界，避免留下孤儿引用。

### 固定数据

- 区域：`Port_Aster`
- 玩家：维修员，非自动决策，100 Credits
- NPC 1：上班者，自动决策，高工作责任，中等饥饿
- NPC 2：旅行者，自动决策，未来 120 分钟后出发
- 机会：餐食 `-15 Credits/30m`、班次 `+120/480m`、休息 `0/480m`、诊疗 `-80/120m`、危险检查 `+300/180m/Risk` 

### 创建顺序

1. 检查 World 和所有依赖子系统。
2. 若本世界已初始化，返回 false 和 `AlreadyCreated`。
3. 用 Seed 和固定标签派生 GUID，禁止 `NewGuid()` 造成不可复现。
4. 创建区域快照。
5. 创建三个人。
6. 为三人创建因果状态，CurrentRegion 均为 `Port_Aster`。
7. 注册机会，并将可重复公共服务设为 `AvailableUses=-1`。
8. 创建安全旅行和危险承诺。
9. 任一步失败时返回具体步骤；M4 前不尝试复杂回滚，PIE 重开恢复。
10. 所有成功后才设置 `bCreated=true`。

### 验收断言

相同 Seed 的所有 ID 和最终结果一致；第二次调用明确失败；缺少任一子系统时不产生崩溃。

## M0-IC06 调试快照聚合器

对应工作包：`M0-WP05`。

### 新建文件

- `WorldSimDebugTypes.h`
- `WorldSimDebugSubsystem.h/.cpp`

### 快照结构

`FPersonDebugSnapshot` 包含身份、LifeState、CurrentRegion、需求值、ChosenGoal、ChosenReason、ActiveActivityId、ActiveCommitmentIds、KnownMessageCount。

`FWorldDebugSnapshot` 包含世界时间、人物数组、活动数组、承诺数组、最近事件、公共消息数量。

### API

```cpp
UFUNCTION(BlueprintPure)
FWorldDebugSnapshot BuildSnapshot(const FSimulationDebugLimits& Limits) const;
```

### 聚合算法

1. 从 PersonSubsystem 获取有界人物列表。
2. 对每个人读取因果状态和最后一次 DecisionTrace，不主动重新决策。
3. 建立 `PersonId -> ActiveActivity` 临时 Map，避免人物循环内反复扫描。
4. 建立 `PersonId -> Commitments` 临时 MultiMap。
5. 查询最近事件和知识数量。
6. 所有数组按人物/时间稳定排序。
7. 快照只复制值，不保存 UObject 指针。

### 验收断言

构建快照前后所有容器数量和世界时间不变；缺少可选数据时使用空值，不触发决策或写事实。

## M0-IC07 Blueprint 冒烟关卡

对应工作包：`M0-WP05`。

### UE 资产

- `/Game/WorldSim/Maps/L_WorldSimSmoke`
- `/Game/WorldSim/UI/WBP_WorldSimDebug`
- `/Game/WorldSim/Blueprints/BP_WorldSimSmokeController`

### Widget 内容

世界时间、人物表、当前目标、活动剩余时间、承诺状态、最近事实、公共消息；按钮为初始化、推进 10 分钟、推进 60 分钟、推进 1440 分钟、刷新快照。

### 蓝图流程

1. BeginPlay 只创建 Widget，不自动推进世界。
2. 初始化按钮调用 Bootstrap 并显示失败原因。
3. 推进按钮调用 `AdvanceSimulationMinutes`。
4. 推进完成后显式调用 `BuildSnapshot` 刷新 UI。
5. UI 不直接写任何人物或承诺字段。

### 验收断言

未初始化推进不崩溃；重复初始化有错误文本；三种时间跨度都能刷新；关闭再开 PIE 回到干净状态。

# M1 一致性内核

## M1-IC01 关键时间点调度器

对应工作包：`M1-WP01`。

### 修改文件

- 新建 `WorldSimulationScheduler.h/.cpp`，或在验证前保持为 `WorldSimulationSubsystem` 私有实现
- `WorldSimulationSubsystem.h/.cpp`
- `CommitmentSubsystem.h/.cpp`
- `OpportunityCompilerSubsystem.h/.cpp`

### 新增内部接口

```cpp
FWorldTime FindNextCriticalTime(FWorldTime From, FWorldTime Target) const;
void ProcessAtTime(FWorldTime At);
```

各子系统提供只读 `GetNext...Time(After)`：活动结束、承诺开始/结束、机会开始/过期。

### 推进算法

1. `Target = Current + Minutes`。
2. 从当前时刻查询所有下一关键时间，选最早且不晚于 Target 的时间。
3. 先按经过分钟增加连续需求。
4. 将世界时间设置到关键点。
5. 固定顺序处理：活动结束、承诺风险结算、生命状态、位置切换、承诺开始、机会过期、重新决策、消息发布。
6. 重复直到到达 Target。
7. 每次调用限制最多处理 10000 个关键点；超过时停止并记录 SimulationOverflow 事实。

### 重要规则

- 同一分钟先死亡后决策，因此死者不能再认领机会。
- 同一分钟先结束旧活动再开始硬承诺。
- 每个处理器必须幂等；同一时间重复调用不能重复结算。

### 验收场景

同一初始存档分别执行 `1440 x 1 分钟`、`24 x 60 分钟`、`1 x 1440 分钟`，比较世界时间、人物状态、承诺结果、活动和事实事件签名完全一致。

## M1-IC02 活动状态模型和中断

对应工作包：`M1-WP02`。

### 数据变更

新增 `EWorldActivityState`：`Planned/Active/Paused/Completed/Cancelled/Failed/Interrupted`。

扩展 `FActiveWorldActivity`：`ActivityId`、`StartedAt`、`ExpectedEnd`、`State`、`InterruptionReason`、`ConsumedFraction`、`ProviderId`。

### API

```cpp
bool InterruptActivity(FGuid PersonId, FName Reason, FWorldTime At);
bool CompleteActivity(FGuid PersonId, FWorldTime At);
```

### 中断算法

1. 找到人物当前 Active 活动；不存在返回 false。
2. `ConsumedFraction = clamp((At-StartedAt)/(ExpectedEnd-StartedAt))`。
3. 根据 Goal 查中断策略表。
4. Eat：按比例结算已消费食物和费用，剩余效果不结算。
5. Work：按合同规则决定部分工资或无工资，并生成缺勤事实。
6. Medical：根据程序阶段决定无效果、部分治疗或并发症。
7. Travel：不能普通中断，只能由交通服务改道、取消或事故处理。
8. 标记 Interrupted，记录事实，释放人物排他槽。

### 验收断言

硬承诺开始时当前工作被中断；中断活动不会随后再次自然完成；同一活动重复中断只产生一条事实。

## M1-IC03 Presence 规范时间线

对应工作包：`M1-WP03`。

### 修改文件

- `PresenceSubsystem.h/.cpp`
- `CommitmentSubsystem.cpp`
- `LocalPersonManagerSubsystem.cpp`

### 数据规则

每个人的 Presence 区间采用半开区间 `[Start, End)`；历史区间不可覆盖；未来区间允许取消后重建；任意时间最多一个非 Unknown 状态。

### API

```cpp
bool ReplaceFuturePresence(FGuid PersonId, FWorldTime From, const TArray<FPresenceInterval>& NewIntervals, FString& OutReason);
bool TryGetPresenceAt(FGuid PersonId, FWorldTime At, FPresenceInterval& OutInterval) const;
bool ValidateTimeline(FGuid PersonId, FString& OutReason) const;
```

### Replace 算法

1. 拒绝无效人物、空区域的 Present、End<=Start。
2. 保留 `End<=From` 的历史区间。
3. 截断跨越 From 的当前区间到 From。
4. 删除 `Start>=From` 的未来区间。
5. 合并新数组并按 Start 排序。
6. 检查相邻区间重叠；发现重叠则不提交。
7. 相邻且 Region/State 相同的区间合并。
8. 全部验证后原子替换人物时间线。

### 承诺映射

- 承诺开始前：Origin Present
- `[PlannedStart, PlannedEnd)`：InTransit，Region 使用载具/航线区域
- 成功结束后：Destination Present
- 取消：恢复 Origin 或按取消位置生成 Present
- 失踪：Unknown/Missing 专用状态，不允许普通实例化
- 死亡：从确认时间开始 Deceased

### 验收断言

创建重叠区间被拒绝且旧时间线不变；边界时刻只命中一个区间；LocalPersonManager 只实例化 Present。

## M1-IC04 承诺风险结果与生命关闭

对应工作包：`M1-WP04`。

### 修改文件

- `CommitmentSubsystem.cpp`
- `MotivationSubsystem.cpp`
- `WorldSimulationSubsystem.cpp`
- `PresenceSubsystem.cpp`

### 风险算法规范

`Seed = Hash(WorldSeed, CommitmentId, PlannedEnd.Minute, OutcomeVersion)`。OutcomeVersion 写入常量，算法修改时必须迁移旧存档结果。

阈值保持单调且总和不超过 1：死亡 `Risk^2*0.08`，失踪追加 `Risk*0.10`，受伤追加 `Risk*0.30`，普通失败追加 `Risk*0.20`，其余成功。

### 终局处理顺序

1. 若 Outcome 已非 None，直接返回，保证幂等。
2. 计算固定 Roll 并写 Outcome。
3. 写 Commitment State。
4. 更新 LifeState、Health、Stress 和 CurrentRegion。
5. 中断人物所有活动。
6. 取消死亡者全部未来非终态承诺；失踪者改为 Delayed 或 Failed。
7. 重建 Presence 未来区间。
8. 写一条终局事实。
9. 根据可观测性发布官方消息，而不是默认让所有人知道。

### 验收场景

固定生成五个强制结果测试数据，分别断言成功、失败、受伤、失踪、死亡；重复结算事件数不增加；死者无法被实例化或决策。

## M1-IC05 事实幂等与因果引用

对应工作包：`M1-WP05`。

### 数据变更

扩展 `FWorldEvent`：`CauseEventId`、`SourceCommitmentId`、`SourceActivityId`、`Visibility`、`SchemaVersion`、`IdempotencyKey`。

### 写入算法

1. 调用方根据业务主体和阶段生成 IdempotencyKey，例如 `Commitment:{Id}:Outcome`。
2. TruthLedger 维护 `IdempotencyKey -> EventId` 内部索引。
3. 相同 key 再次写入时返回原 EventId，不新增事件。
4. CauseEventId 必须存在或为空；不存在时拒绝写入。
5. 事件一旦写入不可修改；调查结论写新事件并引用原事件。

### 验收断言

同一承诺结果处理两次只有一个终局事件；纠正消息不改旧事实；无效 Cause 被拒绝并提供原因。

## M1-IC06 信息传播和个人知识

对应工作包：`M1-WP06`。

### 数据变更

扩展 `FWorldMessage`：`PublisherId`、`PublishedAt`、`AvailableAt`、`ChannelType`、`CorrectionOfMessageId`、`RegionId`。

扩展 `FMessageKnowledge`：`LearnedFromId`、`LearningMethod`、`LastUpdatedAt`。

### 发布算法

1. 事实发生不自动创建消息，先由组织/目击者决定是否发布。
2. 发布者必须知道关联事实或消息。
3. 计算 Channel 的传播延迟并设置 AvailableAt。
4. 公共消息只进入地区媒体池，不直接写入每个人知识。
5. 人物阅读终端、收到私信或交谈时调用 LearnMessage。
6. LearnMessage 校验当前时间不早于 AvailableAt。
7. 相同消息重复学习只更新来源和置信度，不新增重复记录。
8. Correction 消息保留旧知识并更新当前 belief 状态。

### Blueprint 查询边界

玩家 UI 使用 `GetKnownMessages(PlayerId)`；新闻终端使用 `GetAvailablePublicMessages(RegionId, Now)`；调试 UI 才允许直接看事实层。

### 验收场景

1. 远方死亡后事实立即存在，玩家知识为空。
2. 新闻到达前终端查不到消息。
3. 玩家阅读后出现知识记录。
4. 失踪消息被获救消息纠正，旧消息仍可查看。

## M1-IC07 一致性审计器

对应工作包：M1 全部工作包的统一验收支持。

### 新建文件

- `WorldSimConsistencySubsystem.h/.cpp`
- `WorldSimConsistencyTypes.h`

### 问题结构

`FConsistencyIssue`：Severity、Code、PersonId、CommitmentId、ActivityId、AtTime、Description。

### API

```cpp
TArray<FConsistencyIssue> AuditPerson(FGuid PersonId) const;
TArray<FConsistencyIssue> AuditWorld(int32 MaxIssues = 1000) const;
```

### 审计规则

1. Presence 区间不可重叠。
2. 排他活动不可重叠。
3. 活动不可跨越硬承诺，除非有允许标记。
4. Deceased 人物没有 Active 活动或未来普通承诺。
5. Missing 人物不能有 Present 区间。
6. CurrentRegion 与当前 Presence 一致。
7. Commitment Outcome 与终态匹配。
8. Knowledge 引用的 Message 必须存在。
9. Message 引用的 Event 必须存在。
10. 规范 ID 在各自主表内唯一。

### 运行方式

调试按钮手动运行；加载存档后自动运行；开发构建可在每次大跳时运行，Shipping 默认关闭全世界审计。

### 完成证据

人为注入十类错误均被对应 Code 捕获；正常 Demo World 返回空问题数组。
