# 阶段1~3 实施计划（可直接落地）

## 说明
当前工作区只检测到文档文件，没有现成 `.uproject`。本计划先按“可接入现有工程/新建工程”两种模式提供。

---

## 阶段1：UE5工程骨架与最小接口（第1~3天）

### A. 工程搭建

- 情况1（有现有UE工程）
  - 在现有工程 `Source/<GameName>/` 下新增 `WorldSimCore` 模块（建议独立模块）
  - 在 `.Build.cs` 增加依赖：`Core`, `CoreUObject`, `Engine`, `InputCore`, `Json`, `JsonUtilities`
  - 增加日志类别：`LogWorldSim`

- 情况2（无UE工程）
  - 新建 UE C++ 项目（Basic）
  - `项目名`: `WorldSimDemo`
  - 后续所有文件路径按 `Source/WorldSimDemo/` 放置

### B. 目录与文件清单（阶段1）

- `Source/WorldSimDemo/WorldSimDemo.h`
- `Source/WorldSimDemo/WorldSimDemo.cpp`
- `Source/WorldSimDemo/WorldSimDemoGameInstance.h/.cpp`（可选）
- `Source/WorldSimDemo/WorldTimeSubsystem.h/.cpp`
- `Source/WorldSimDemo/RegionSnapshotSubsystem.h/.cpp`
- `Source/WorldSimDemo/CommitmentSubsystem.h/.cpp`
- `Source/WorldSimDemo/PresenceSubsystem.h/.cpp`
- `Source/WorldSimDemo/TruthLedgerSubsystem.h/.cpp`
- `Source/WorldSimDemo/LocalPersonManager.h/.cpp`
- `Source/WorldSimDemo/WorldSimTypes.h`（统一枚举和结构体）
- `Source/WorldSimDemo/WorldSimBlueprintFunctionLibrary.h/.cpp`
- `Content/Maps/Dev/WorldSimDev.umap`
- `Content/Blueprints/BP_WorldSimDebugController`

### C. 阶段1最小接口（先实现，先能编译）

#### `WorldSimTypes.h`
- 定义：
  - 枚举：`EExistenceState`, `EPersonActivityState`, `ECommitmentType`, `EKnowledgeConfidence`
  - 结构：
    - `FRegionSnapshot`
    - `FPersonLiteState`
    - `FPersonFullState`
    - `FCommitmentEvent`
    - `FPresenceInterval`
    - `FWorldEvent`
    - `FPlayerKnowledgeItem`

#### `UWorldTimeSubsystem`
- `FDateTime GetNow() const`
- `void AdvanceWorldTime(float DeltaSeconds)`
- `void SetNow(FDateTime NewNow)`
- `FDateTime GetMidnight(FDateTime InTime)`
- `int32 DaysSinceStart()`

#### `URegionSnapshotSubsystem`
- `void EnsureRegion(int32 RegionId)`
- `FRegionSnapshot GetSnapshot(int32 RegionId) const`
- `void AdvanceRegion(int32 RegionId, float DeltaHours)`
- `void ApplyMicroDelta(int32 RegionId, int32 DeltaPopulation, int32 DeltaTraffic)`

#### `UCommitmentSubsystem`
- `FGuid CreateCommitment(const FCommitmentEvent& InEvent)`
- `bool CancelCommitment(FGuid CommitmentId)`
- `bool ResolveDueCommitments(const FDateTime& Now)`
- `TArray<FGuid> GetCommitmentsForPerson(FGuid PersonId) const`

#### `UPresenceSubsystem`
- `bool CanAppear(FGuid PersonId, int32 RegionId, const FDateTime& At) const`
- `bool LockPresence(const FPresenceInterval& Interval)`
- `void ReleasePresence(FGuid PersonId, const FDateTime& At)`
- `bool IsActiveHere(FGuid PersonId, int32 RegionId, const FDateTime& At) const`
- `TArray<FGuid> GetPeopleInRegion(int32 RegionId) const`（先返回最近一次活跃/承诺中的人）

#### `UTruthLedgerSubsystem`
- `void AppendEvent(const FWorldEvent& Event)`
- `bool TryGetLatestState(FGuid PersonId, EExistenceState& OutState, FWorldEvent& OutEvidence) const`
- `TArray<FWorldEvent> QueryEvents(FGuid PersonId, FDateTime From, FDateTime To) const`

#### `ULocalPersonManager`
- `void ActivateNearby(int32 RegionId, int32 MaxCount)`
- `void Simulate(float DeltaSeconds)`
- `void DeactivateOutOfRange(const TArray<FGuid>& VisibleNow)`
- `void SpawnOrUpdatePersonFull(FGuid PersonId, const FPersonLiteState& Lite)`

#### 蓝图函数库
- `UFUNCTION(BlueprintPure)` `FDateTime GetWorldNow()`
- `UFUNCTION(BlueprintPure)` `FRegionSnapshot GetRegionSnapshot(int32 RegionId)`
- `UFUNCTION(BlueprintPure)` `bool CanNpcExistHere(FGuid PersonId, int32 RegionId)`
- `UFUNCTION(BlueprintPure)` `EExistenceState QueryPersonTruthState(FGuid PersonId)`

### D. 阶段1验收目标
- 能编译通过
- 蓝图可读取世界时间与区域快照
- 可以调用 `CanNpcExistHere` 得到“可见约束”结果

---

## 阶段2：世界真相层（第4~7天）

### A. 文件新增
- `Source/WorldSimDemo/WorldSimScheduler.h/.cpp`
- `Source/WorldSimDemo/CommitmentEvaluator.h/.cpp`

### B. 阶段2接口扩展

#### 在 `UCommitmentSubsystem` 中
- `bool UpdateCommitmentState(FGuid CommitmentId, const FString& NewState, const FString& Reason)`
- `TArray<FGuid> GetDueCommitments(const FDateTime& Now, float LookAheadHours = 1.f)`

#### 在 `UPresenceSubsystem` 中
- `TOptional<FPresenceInterval> GetInterval(FGuid PersonId) const`
- `void ResolveConflict(FGuid PersonId)`

#### 在 `UTruthLedgerSubsystem` 中
- `void MarkEventConflict(const FGuid& PersonId, const FString& ConflictReason)`
- `bool HasHardCommitmentConflict(FGuid PersonId, int32 RegionId, const FDateTime& At)`

#### 新增 `FWorldEvent::EventKind`
- `Promised`, `Departed`, `Arrived`, `OperationStart`, `OperationEnd`, `Returned`, `Survived`, `Dead`, `Cancelled`, `Delayed`

### C. 真相执行流程（最小）
1. 对话事件 -> `CreateCommitment`
2. 调度器在每小时刻执行：
   - 检查可到场条件（资源/交通）
   - 成功则生成 `Departed/Arrived/OperationStart` 及 `PresenceInterval`
   - 失败则生成 `Delayed/Cancelled`
3. 执行结束生成 `Returned/Survived/Dead`
4. 真相结果仅写入 `TruthLedger`

### D. 阶段2验收目标
- 承诺事件到日志为一条可回放链
- 同一时刻同一人只允许一条活动存在区间
- 可通过日志重建“他是否在前线/回家/未知”

---

## 阶段3：角色出生与日常流转（第8~13天）

### A. 文件新增
- `Source/WorldSimDemo/CharacterIdentityTypes.h/.cpp`
- `Source/WorldSimDemo/PlayerBirthService.h/.cpp`
- `Source/WorldSimDemo/DailyRoutineSystem.h/.cpp`

### B. 关键结构

#### 人物档案（最小）
- 性别、外观参数、文化标签、职业候选、财富等级
- 出生地候选与权重参数

#### 日常状态
- `EatTimeWindow`
- `CommutePathId`
- `WorkShiftWindow`
- `RestWindow`
- `NextDayRerollPolicy`

### C. 核心接口
- `FGuid GeneratePlayerProfile(const FString& Seed, const FPlayerCharacterInput& Input)`
- `int32 SelectBirthRegion(const FPlayerCharacterProfile& Profile)`
- `void BuildInitialLiteState(FGuid PlayerId, int32 BirthRegionId)`
- `TArray<FPersonFullState> GenerateDailySchedule(const FPlayerCharacterProfile& Profile, FDateTime Day)`
- `void AdvanceToNextDay(FGuid PlayerId)`
- `void ApplyDailyInterrupts(FGuid PlayerId, const FRegionSnapshot& Snapshot)`

### D. 日常行为状态机（最小）
`Eating -> CommuteToWork -> Work -> MealBreak -> Work -> CommuteHome -> Rest -> Sleep`

### E. 阶段3验收目标
- 角色可创建，且有初始出生地/职业/作息
- 连续1个游戏日模拟无矛盾
- 交通/人口快照有可观测变化

---

## 阶段1-3 每日工作清单（可直接执行）

### Day 1
- 创建/接入 UE5 工程与 WorldSim 模块
- 写 `WorldSimTypes.h` 与日志
- 写 `WorldTimeSubsystem`、蓝图查询函数
- 先能在编辑器输出当前世界时间

### Day 2
- 完成 `RegionSnapshotSubsystem`
- 完成 `CommitmentSubsystem` 与 `PresenceSubsystem` 的基本骨架
- 蓝图读取区域快照和“NPC可出现”判断

### Day 3
- 完成 `TruthLedgerSubsystem` 最小事件日志
- 连接 `WorldTime + Commitment + Presence + Ledger` 的基本链路
- 完成一个开发者蓝图测试关卡（调试UI按钮触发测试）

### Day 4
- 增加 `CommitmentEvaluator`：承诺到达执行
- `Presence` 加锁/解锁与冲突检测
- 完成第一条日志链（承诺->出发/取消）

### Day 5
- 引入 `PlayerIdentity/Profile` 数据结构
- 完成人物出生地匹配算法（可配置权重）
- 输出初始 `PersonLiteState`

### Day 6
- 生成当天日程（吃饭-通勤-工作-休息）
- 增加 `LocalPersonManager::Simulate` 的最小版（离线 tick）
- 本地可视化：打印/日志看人物活动

### Day 7
- 复盘：修复边界条件（时间边界、状态回写）
- 准备阶段2-3联调报告：一条NPC承诺完整流转
- 为下一阶段预留 `PersonLite/Full` 升级点和通信事件点

---

## 风险控制
- 第1~3天只做最小可执行版本，避免渲染与玩法耦合
- 每个系统只保留最小字段；参数以 `DataAsset` 可配置
- 当无法实时计算时使用默认行为兜底（例如：通勤失败=>改签事件）
