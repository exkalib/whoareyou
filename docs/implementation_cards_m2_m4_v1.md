# M2-M4 代码级实施卡 V1

# M2 单人单城市生活

## M2-IC01 身份数据与创建服务

对应：`M2-WP01`。

修改文件：扩展 `WorldSimTypes.h`；新建 `PlayerIdentitySubsystem.h/.cpp`、`IdentityTypes.h`。

数据结构：`FIdentityCreationRequest` 保存姓名、性别、出生年、外观种子、背景模板、期望职业；`FIdentityCreationResult` 返回 PersonId、HouseholdId、JobId、HomeRegion、SpawnContext 和失败码；`FBackgroundTemplate` 保存允许年龄、教育、资产区间、辖区和职业入口。

API：`ValidateIdentity(Request, OutIssues)` 纯查询；`CreateIdentity(Request, OutResult)` 只在校验成功后写数据；`TryGetPlayerPersonId()` 返回本世界玩家身份。

算法：

1. 标准化姓名和模板 ID，拒绝空姓名、非法年龄和未知模板。
2. 用 WorldSeed、外观种子和模板派生稳定 PersonId。
3. 从模板产生教育、初始 Credits、家庭角色和职业资格。
4. 先构造全部 DTO，不立刻写各子系统。
5. 按 Person、Household、Employment、Motivation、Knowledge 顺序提交。
6. 任一步失败时调用本次创建的补偿删除接口；没有补偿接口前禁止暴露 CreateIdentity。
7. 成功后写 `IdentityCreated` 事实并保存 PlayerPersonId。

验收：同请求同种子结果一致；非法年龄不产生半个人物；一个 World 只能有一个玩家身份；外观变化不改变家庭和职业结果。

## M2-IC02 家庭、住宅与因果出生

对应：`M2-WP02`。

新建：`HouseholdSubsystem.h/.cpp`、`SpawnResolutionSubsystem.h/.cpp`、`HouseholdTypes.h`。

结构：`FHouseholdRecord` 包含成员、监护关系、共享预算、ResidenceId；`FResidenceRecord` 包含 RegionId、BuildingId、Capacity、Occupants、AccessPolicy；`FSpawnCandidate` 包含位置、原因、合法时间、分数。

API：`ResolveSpawn(PersonId, At, OutCandidate, OutReason)`；`RegisterResidence`；`AssignResident`。

出生筛选顺序：

1. 若 Presence 显示 InTransit，候选只能来自载具内部 SpawnPoint。
2. 若当前有医院/拘留/工作硬承诺，查询对应设施 SpawnProvider。
3. 否则查询家庭住宅且验证成员权限、容量和当前时间。
4. 无住宅时查询雇主宿舍、公共安置和合法旅馆。
5. 对候选按硬承诺匹配、家庭归属、距离、稳定性排序。
6. 选中后写 CurrentRegion 和 Present 区间，但具体 FVector 只交给表现层。

失败处理：所有候选均失败时返回 `NoLegalSpawn`，不得默认放到世界原点；调试模式可使用显式 `DebugFallback` 并写告警事实。

验收：维修工在宿舍/住宅出生，在途旅客在船内出生，被拘留者不能在家出生，满员住宅不会超容量。

## M2-IC03 设施与机会提供者接口

对应：`M2-WP03`。

新建：`WorldServiceProviderSubsystem.h/.cpp`、`ServiceProviderTypes.h`。

结构：`FServiceProvider` 包含 ProviderId、RegionId、ServiceTags、OpeningIntervals、Capacity、Inventory、PriceRules、AccessRules；`FServiceReservation` 包含 PersonId、OpportunityId、资源预留和过期时间。

API：`RegisterProvider`、`RefreshProviderOpportunities(At)`、`ReserveService`、`CommitServiceUse`、`ReleaseReservation`。

刷新算法：

1. 只处理到达刷新时间或状态变更的 Provider，不逐帧重建。
2. 根据营业时间、库存、人员、设备和政策计算可服务量。
3. 为食物、床位、诊疗和休闲生成 FWorldOpportunity。
4. Opportunity 的价格通过负 CreditReward 表示，Duration 来自服务定义。
5. 认领时先原子预留容量和库存，再创建活动。
6. 活动达到实际消耗点后 Commit；取消时按规则 Release 或扣除手续费。

守恒断言：没有库存不能生成餐食；两人不能抢最后一个床位；服务失败不会同时扣全款并返还全款。

## M2-IC04 生理需求与消费结算

对应：`M2-WP03`。

修改：扩展 `FPersonCausalState`；新建 `PhysiologySubsystem.h/.cpp`。

字段：Hydration、Nutrition、DigestionLoad、Bladder、Bowel、IllnessSeverity、SleepDebt；M2 首版只对玩家和热 NPC 展开，温 NPC 继续使用 Hunger/Fatigue 汇总值。

API：`AdvanceDetailedPhysiology(PersonId, Minutes)`、`ConsumeItem(PersonId, ItemDefinition, Fraction)`、`GetPhysiologyNeeds`。

算法：按关键生理时间点批量积分，不逐 Tick 模拟；每口食物只在动画事件 `ConsumeFraction` 到达时减少容器并增加摄入；消化队列保存摄入批次、营养、水分、病原风险和预计阶段时间；远离后把队列汇总为 Nutrition、DigestionLoad 和下一排泄窗口。

验收：盘中食物每次入口后减少；中断吃饭只结算已入口部分；不同食物产生不同消化窗口；降级再升级不恢复已吃掉食物。

## M2-IC05 工作合同与班次生成

对应：`M2-WP04`。

新建：`EmploymentSubsystem.h/.cpp`、`EmploymentTypes.h`。

结构：`FEmploymentContract` 包含 EmployerId、PersonId、RoleId、WorkRegion、RecurringSchedule、PayRule、AbsenceRule；`FWorkShiftInstance` 保存具体班次和结算状态。

算法：

1. 每日滚动窗口只生成未来 7 天班次，避免无限日程。
2. 合同规则展开为 ShiftInstance 和 Hard/Soft Commitment。
3. 班前按交通时间生成 Prepare/Commute Opportunity。
4. 到岗必须同时满足 Present、时间、访问权限。
5. 工作活动按有效在岗分钟累计完成度。
6. 班次结束根据完成度、合同和事故结算工资。
7. 迟到、早退、缺勤、请假分别写事实并更新雇主关系。

验收：完整班次获得正确工资；迟到只计算有效分钟；请假不等同旷工；休息日不生成普通班次；班次不会无限预生成。

## M2-IC06 日常机会编译与选择解释

对应：`M2-WP03`、`M2-WP04`。

修改：`MotivationSubsystem`、`OpportunityCompilerSubsystem`。

新增 `FOpportunityScoreBreakdown`：NeedScore、ObligationScore、RelationshipScore、DistanceCost、MoneyCost、RiskCost、TimeConflictCost、LongTermValue、FinalScore。

算法：先做硬过滤（生命、位置、权限、时间、资源、承诺），再做软评分；每项分数必须记录来源；没有合法机会时返回 Idle/SeekHelp，不得从不匹配机会中硬选；增加小幅确定性偏好噪声，种子由人物、决策时间桶和机会 ID 组成。

验收：想吃饭但没钱时会选择家庭食物、求助或工作，而不是认领医院；调试 UI 能逐项解释胜出原因。

## M2-IC07 两日垂直切片验收

对应：`M2-WP05`。

固定场景：一个住宅、维修站、餐厅、诊所、公交节点；玩家加 10 NPC；时间从 Day 1 06:00 到 Day 3 06:00。

执行：正常跑一天；第二天玩家占用最后餐食、导致一名 NPC 改道；制造一次迟到和一次受伤；分别按 1 分钟、1 小时和关键点跳时运行；中途保存加载。

断言：金钱总变化等于工资与消费；餐食库存不为负；Presence 无重叠；死者/失踪者不活动；同初始种子最终事实签名一致；加载前后状态一致。

# M3 局部社会与执法

## M3-IC01 稀疏关系图

对应：`M3-WP01`。

新建：`RelationshipSubsystem.h/.cpp`、`RelationshipTypes.h`。

结构：`FRelationshipEdge` 使用有向 From/To，保存十个维度、LastInteractionAt 和最多 N 条关键 CauseEventId；没有关系的两人不创建边。

API：`ApplyRelationshipEffect(EventId, From, To, Effect)`、`GetRelationship`、`GetStrongRelations(PersonId, Dimension, Threshold, Limit)`。

算法：事实规则表将帮助、伤害、欺骗、照顾、拒绝映射到多个维度增量；值使用衰减边界但不自动回到零；普通互动只更新汇总，重大事件保留 Cause；双向关系分别更新。

验收：A 信任 B 不代表 B 信任 A；删除冷普通互动不删除重大背叛原因；重复处理同一 EventId 不重复加值。

## M3-IC02 组织、岗位和权限

对应：`M3-WP02`。

新建：`OrganizationSubsystem`、`OrganizationDecisionSubsystem`、`OrganizationTypes.h`。

结构：Organization、Department、Role、Membership、Asset、BudgetAccount、PermissionGrant、DecisionTrace。

决策周期：状态变化事件立即唤醒；普通组织每 6 小时或每天评估；候选行动由需求模板生成；硬过滤预算/权限/人员；效用考虑使命、存续、派系和风险；胜出行动转换为岗位、合同、采购、消息或承诺。

验收：单位缺勤后先调班，无人可调才招聘或减产；预算不足不能凭空招聘；每次行动可显示候选和拒绝原因。

## M3-IC03 感知记录

对应：`M3-WP03`。

新建：`PerceptionEvidenceSubsystem`、`EvidenceTypes.h`。

结构：`FObservableOccurrence`、`FWitnessObservation`、`FEvidenceRecord`。观察与事实分离，观察可以错误。

流程：动作系统提交可观察发生；查询附近热 NPC 和设备；按视线、距离、噪声、注意力、遮挡、传感器状态计算质量；确定性采样是否观察到；产生 Observation；需要持久化的物理/数字痕迹生成 Evidence；证据记录完整性和保管者。

验收：密室无传感器行为不产生远方目击者；摄像头断电无视频；同一存档重复加载观察结果一致。

## M3-IC04 报警和真实调度

对应：`M3-WP04`。

新建：`EmergencyDispatchSubsystem`、`DispatchTypes.h`。

流程：观察者根据威胁、关系、职责、恐惧选择报警/帮助/逃离；报警生成 CallRecord；辖区按事件等级、距离、可用人员和装备选择 ResponseUnit；创建从单位当前位置到现场的旅行承诺；到场前案件状态为 EnRoute；交通延误真实影响到场；接触后交给执法策略。

验收：没有报警或自动传感器不立即出警；警察从真实位置出发；同一单位不能同时响应两个排他任务。

## M3-IC05 案件和调查行动

对应：`M3-WP05`。

新建：`CaseSubsystem`、`InvestigationSubsystem`、`CaseTypes.h`。

结构：Case、Lead、Hypothesis、SuspectBelief、InvestigationAction、CustodyEntry。

算法：案件由报警、尸体、失踪或审计触发；线索只引用已知 Observation/Evidence；侦探按信息价值、成本、权限和时效选择访问、取证、讯问、检验；结果更新假设概率；达到起诉阈值仍需合法证据；案件可悬置并在新证据后重开。

验收：侦探不能读取 TruthLedger 中未发现的凶手 ID；销毁证据会改变路径；误导证据可造成怀疑但不是必然定罪。

# M4 多精度、存档与长期性能

## M4-IC01 Simulation LOD 分类器

对应：`M4-WP01`。

新建：`SimulationLODSubsystem`、`SimulationLODTypes.h`。

结构：`ESimulationTier Hot/Warm/Cold`、`FSimulationImportance`、`FTierTransitionRecord`。

评分：玩家距离、是否可见、关系强度、活动/承诺、案件、剧情标记、最近接触；Hot 和 Warm 都设硬上限；超限时按重要度稳定淘汰；死亡或重要历史不强制 Hot，只保证 Warm 记录。

降级：Hot Actor 状态写回 PersonFull，再汇总到 PersonLite/CausalState，销毁 Actor；Warm 到 Cold 将普通个体并入 Household/Population Cohort，但保留重要 ID 索引。

升级：从种子、压缩状态、区域快照和关键历史重建；重建细节不得改变身份、位置、资产和关系。

验收：反复进出区域不复制 NPC、不恢复库存、不改变外观种子；热人物数始终在预算内。

## M4-IC02 区域宏观状态转移

对应：`M4-WP02`。

扩展 `FRegionSnapshot`：PopulationCohorts、JobsBySector、Housing、FoodStock、MedicalCapacity、Security、TransportLoad、PriceIndex、PolicyIds、LastSimulatedAt。

每日算法：生产消耗资源；就业产生收入和产出；住房/物价影响迁移；医疗与事故影响健康聚合；治安和压力生成案件率；组织政策修正参数；从聚合变化中只抽样重要事件；最后写新 Snapshot 和摘要事实。

细化：玩家进入时，按 Cohort 权重生成局部 NPC 候选；被玩家接触后分配永久 PersonId 并从匿名 Cohort 扣减一个。

验收：人口和资源无负数；聚合转个体后总人口守恒；一年模拟不创建百万 Person UObject。

## M4-IC03 存档 DTO 与原子保存

对应：`M4-WP03`。

新建：`WorldSimSaveGame.h/.cpp`、`WorldSimPersistenceSubsystem.h/.cpp`、`WorldSimSaveTypes.h`。

结构：Header 包含 Magic、SchemaVersion、EngineVersion、WorldSeed、SavedAt、Checksum；各子系统 DTO 只含 UPROPERTY 值类型，不含 UObject 指针。

保存流程：暂停规范推进；各系统 ExportSnapshot；运行一致性审计；序列化到临时槽；读取临时槽并校验 Header/Checksum；成功后替换正式槽；恢复推进。失败保留旧正式槽。

加载流程：读 Header；逐版本迁移 DTO；创建干净 World；按 Time/Region/Person/Organization/Truth/Commitment/Presence/Activity/Knowledge 顺序导入；重建索引；运行审计；成功后允许 BeginPlay。

验收：保存中崩溃不损坏旧档；未知更高 Schema 拒绝加载；加载后规范状态哈希一致。

## M4-IC04 事件冷热归档

对应：`M4-WP04`。

新建：`WorldHistoryLifecycleSubsystem`。

策略：近 30 天完整热事件；重要人物、死亡、承诺、案件、所有权和玩家知识永久完整；普通旧互动按人物/区域/周压缩成 SummaryEvent；原 EventId 到 SummaryId 保留重定向；消息引用的重要事实禁止压缩到不可查询。

归档按每日预算分批执行，不在单帧扫描全历史；维护 Subject/Region/Type/Time 索引；所有索引可从规范数组重建。

验收：压缩前后人物关键历史查询语义一致；十年普通事件存储增长受控；中断归档后可继续。

## M4-IC05 性能预算与压测场景

对应：`M4-WP04`。

新增 `WorldSimMetricsSubsystem`，记录每个系统本次处理实体数、微秒、队列长度、事件数、存档字节。

固定档位：50 Hot、10,000 Warm、1,000,000 聚合人口；推进一天、一个月、一年；接触 1000 个不同 NPC；连续保存 100 次。

失败阈值先作为配置而非虚构绝对数；首次 Windows 机器记录基线，再确定帧预算。必须观察复杂度趋势：远方人口增加 10 倍不能让逐帧个体循环增加 10 倍。

验收输出：CSV/日志包含场景、硬件、引擎版本、每系统耗时、峰值内存、存档大小和一致性问题数。
