# M5-M9 代码级实施卡 V1

# M5 行星交通

## M5-IC01 时间依赖交通图

对应：`M5-WP01`。新建 `TransportGraphSubsystem`、`TransportTypes.h`。

类型：Node 保存 RegionId、站点类型、开放时间；Edge 保存 OperatorId、From/To、基础时长、班次规则、容量、价格、许可、风险；`FRouteLeg` 和 `FRoutePlan` 保存每段出发/到达、换乘和总成本。

API：`RegisterNode/Edge`、`FindRoutes(Request, MaxRoutes)`、`NotifyDisruption`。

路线算法：以“最早可到达时间”为状态执行时间依赖 Dijkstra/A*；展开边时查找请求到达节点后的下一班次；硬过滤许可、载具状态、容量和营业时间；成本向量包含到达、价格、换乘、风险，按人物偏好加权；返回前 K 条非支配路线并附逐项解释。

错误：起终点相同返回零段路线；无路返回明确 Reason；时间溢出和负时长注册时拒绝；路网版本变化使未购票 Plan 失效。

验收：早班快线、廉价慢线、低风险线均能按偏好胜出；错过一分钟自动选择下一班而非倒退时间。

## M5-IC02 票据和容量事务

对应：`M5-WP02`。新建 `TicketingSubsystem`、`TicketTypes.h`。

结构：Ticket 保存 PlanVersion、PassengerId、Legs、Price、State、ReservedUntil；每班次保存 Capacity 和 ReservationIds。

购买事务：再次验证路线；按全行程顺序预留每段座位；任一段失败则逆序释放；检查 Credits；扣款；创建 Ticket；创建旅行承诺；写购票事实。禁止先扣款后发现无座。

取消：按运营规则计算退款；释放未出发段；更新承诺；写取消与退款事实。重复取消幂等。

验收：最后一个座位只能卖给一人；多段第二段失败不会占第一段；余额与运营账户总变化守恒。

## M5-IC03 出发、载具容器和到达

对应：`M5-WP02`。新建 `TravelServiceSubsystem`。

状态：Booked、CheckIn、Boarded、Departed、InTransit、Arrived、Missed、Cancelled、Disrupted。

流程：截止时间验证乘客 Present 于站点；登乘后 Presence 切到 VehicleRegion；出发锁定该段；到达切目标站点；换乘重新进入站点 Present；全程完成后更新 CurrentRegion 和 Commitment。

载具必须是 Region Record，乘客在途时可在车/船内吃饭和互动，但不能认领外部城市机会。TravelService 是唯一允许修改旅行 Presence 的服务。

验收：乘客在交通工具内可见；不能同时出现在起点街道；到站边界只有一个 Presence 命中。

## M5-IC04 延误、改签和失联

对应：`M5-WP02`。

扰动结构：AffectedLeg、Start、ExpectedEnd、CauseEventId、Severity。发生后找所有受影响票据；未出发者重新规划或退款；在途者延长 Vehicle Presence；错过衔接者生成住宿/改签机会；危险事件可将状态改为 Lost 并进入搜救，而不是直接死亡。

验收：延误不会让承诺自动按原时间完成；改签不重复收费；失联只发布失联消息，确认结果后再更新。

# M6 星际社会

## M6-IC01 天体和层级位置

对应：`M6-WP01`。新建 `AstronomySubsystem`、`AstronomyTypes.h`。

类型：StarSystem、CelestialBody、OrbitRegion、GravityWell、WarpBoundary、HierarchicalLocation。规范位置由 ContainerId、LocalFrame、轨道参数/局部坐标组成，不用银河尺度 FVector。

API：注册天体、查询父链、解析某时刻相对位置、验证曲率边界。轨道位置按确定性简化轨道模型计算；表现层把附近父链转换为 UE LWC 坐标。

验收：同一飞船乘员共享移动父容器；时间推进后轨道位置变化但人物舱室位置不漂移。

## M6-IC02 曲率航线和发射许可

对应：`M6-WP01`。新建 `WarpRouteSubsystem`、`WarpTravelService`。

飞船字段：Mass、CoreSeed、Energy、Heat、HullFatigue、NavigationMapVersion、MaintenanceState。请求检查位于 WarpBoundary 外、目标地图有效、能源/散热/维护、许可和航路窗口。

计算：TravelDuration 由距离、设计速度、质量修正和航路稳定度决定；风险来自核心、疲劳、地图误差和外部异常；发射时消耗能源并创建不可普通取消承诺；到达生成泡壁释放和强制检修需求。

验收：地面发射被拒绝；过期地图提高风险或拒绝；航行中不能实时改目标；能源和疲劳真实变化。

## M6-IC03 光速/信使消息路由

对应：`M6-WP02`。新建 `CommunicationNetworkSubsystem`。

Channel：Local、RadioLaser、CourierShip。消息 Envelope 保存 Origin、Destination、SentAt、EarliestArrival、PayloadMessageId、Route、State。

算法：本地即时进入媒体池；光速用天体距离/光速计算到达；信使消息绑定真实船票/航程，船失联则消息不达；到达后才设置 Message.AvailableAt。每个地区按 Subject/Topic 保存已知版本向量，冲突版本都保留来源。

验收：火星不能立刻知道另一恒星战争；信使比光信号早到时先形成一个版本；后到消息不会无条件覆盖高可信新消息。

## M6-IC04 星球贸易、迁移与自治

对应：`M6-WP03`。新建 `InterplanetaryEconomySubsystem`、`ColonyGovernanceSubsystem`。

贸易订单来自区域供需和组织合同；航运者选择有货舱、利润和可承受风险的路线；装货从库存扣除并进入 ShipCargo；卸货后才进入目标库存和结算。迁移要求座位、住所/担保、许可和资金。

自治模型保存本地合法性、资源依赖、通信延迟、中央信任；政策决策只用本地已知版本；中央命令到达后可接受、修改或拒绝并产生政治事实。

验收：货船失踪会造成真实短缺；远殖民地不会读取太阳系最新政策；迁移人口在来源和目标守恒。

# M7 修炼与动作

## M7-IC01 修炼规范数据

对应：`M7-WP01`。新建 `CultivationSubsystem`、`CultivationTypes.h`、DataAssets。

状态：Body、Energy、Insight、Control、Realm、Stage、SourceReserve、InjuryLoad、KnownMethods。每次增长使用 `FProgressContribution` 保存来源 EventId、维度、原始量、效率、最终量，禁止直接 SetExperience。

突破定义 DataAsset 保存最低四维、材料、设施、身体状态、控制挑战和失败表。`EvaluateBreakthrough` 只返回缺口；`BeginBreakthrough` 预留资源并创建活动；结束按固定挑战结果结算。

验收：没有材料不能突破；训练空间只增加 Insight/Control；重复消费同一来源事件被幂等键拒绝。

## M7-IC02 火种系统建议器

对应：`M7-WP02`。新建 `SystemAdvisorSubsystem`。

输入只能是 PlayerKnowledge、扫描结果、身体状态、已知设施和公开机会。先计算成长缺口，再查询 OpportunityCompiler，最后生成 `FSystemRecommendation`，包含依据知识 ID、预期贡献、成本、风险、时间和不确定性。

系统不得直接注册世界机会；找不到已知机会时显示“需要寻找信息/扫描”，不能凭空创建任务坐标。

验收：调试事实层存在但玩家未知的遗迹不会被建议器泄漏；获得消息后建议自然出现。

## M7-IC03 感源动作组件

对应：`M7-WP03`。新建 `CultivationAbilityComponent`、`AbilityTypes.h`、Enhanced Input Mapping。

能力定义：TargetPolicy、Range、Cost、CastTime、CancelWindow、Cooldown、EffectSpec。组件状态：Idle/Aiming/Casting/Recovering/Interrupted。输入只请求能力；服务端/规范层验证 Energy、状态、目标和权限；动画通知在命中时提交 Effect；Effect 产生伤害、移动、证据和资源消耗事实。

首批：扫描、源质闪步、附能攻击、护体。扫描结果写玩家知识；闪步做可预测位移但最终位置通过碰撞验证；所有能力可被硬控制中断并按阶段退款。

验收：按键、手柄均可操作；无 Energy 失败不播放完整命中；墙后非法目标不受伤；技能结果可查询。

## M7-IC04 遗迹事故事件图

对应：`M7-WP04`。新建 `WorldEventGraphSubsystem`、事件 DataAsset。

节点保存 Preconditions、EarliestAt、LatestAt、Actors、Actions、Outcomes；边由事实而非玩家任务完成标记触发。遗迹图节点：维修任务、异常扫描、事故、系统绑定、认证信号、组织调查、深空接收。

处理器低频查询下一可触发节点；节点执行通过服务 API 修改资源/承诺/消息；每节点 IdempotencyKey；错过窗口走替代节点。

验收：玩家不接维修任务时另一维修员可能触发；事故后信号时间线继续；重复加载不重复绑定系统。

# M8 文明与舰队

## M8-IC01 组织低频决策框架

对应：`M8-WP01`。扩展 Organization，新增 `StrategicDecisionSubsystem`。

Blackboard 只保存组织已知情报、资产、人员、预算、目标、约束和派系支持。ActionDefinition 提供 Preconditions、Cost、Duration、ExpectedEffects、Risk。唤醒来源是重大消息或 NextDecisionAt；候选先硬过滤再效用评分；确定性选择后创建预算预留、人员承诺和行动实例。

决策轨迹保存所有候选分项，便于解释。失败仍消耗已经投入的真实资源。

验收：不知道外星舰队的组织不能针对它部署；资源不足的军方只能请求预算或撤退；不会逐帧重算战略。

## M8-IC02 Day 0-120 危机 DataAsset

对应：`M8-WP02`。新建 `DA_RelicPursuitTimeline` 和专用事件动作。

每个节点写：事实前置、默认最早日、可延迟因素、参与者、生成消息、组织决策刺激、后继条件。节点不是固定 cutscene：侦察器可能被发现/捕获/漏过，外交通告可能被公开/封锁，封锁强度取决于舰队和人类准备。

推进使用 M1 关键点调度；Day 120 只是默认全面行动最早点，前序结果可改变方式但不按玩家等级缩放。

验收：四条固定剧本种子分别覆盖无知、早知、隐藏、强力干预；所有路径都有后继状态。

## M8-IC03 战争局势状态机

对应：`M8-WP03`。新建 `ConflictTheaterSubsystem`、`ConflictTypes.h`。

状态不使用单一胜负值，保存双方舰队完整度、补给、情报、控制节点、平民风险、政治支持、派系态度。行动包括侦察、封锁、谈判、袭击、撤离、占领；结算调用真实资产和人员承诺。

达到条件后生成 Occupation、Partition、Treaty、HumanVictory 等局势记录；记录只改变规则、控制和机会提供者，不切换静态结局关卡。

验收：殖民后仍生成工作、检查、抵抗和谈判机会；击败舰队后生成俘虏、技术和外交后果。

# M9 宇宙和表现

## M9-IC01 分层确定性生成器

对应：`M9-WP01`。新建纯数据模块 `WorldGeneration`，避免依赖 Actor。

种子链：UniverseSeed -> GalaxyCellSeed -> SystemSeed -> BodySeed -> BiosphereSeed -> CivilizationSeed。每层使用命名 Salt，新增下游字段不能改变已有上游结果。先生成恒星质量/年龄，再轨道与行星，再气候地质，再生态，再智慧概率和社会历史。

生成结果分 Canonical Summary 和可重建 Detail Recipe；玩家发现后将 Summary 写规范世界，后续版本升级不得静默重滚已发现星球。

验收：字段添加前后已有 ID 和核心物理不变；一万星球批量生成无非法轨道/负资源；差异有物理来源。

## M9-IC02 历史聚合生成

对应：`M9-WP01`。

文明历史按年代步长运行人口、能源、生态压力、制度、技术、战争和接触变量；只有转折生成 HistoricalEvent；关键人物仅在玩家接触或重大事件需要时具名化。技术必须检查前置知识、资源、组织和社会接受，遗迹可提高研究速率但不直接免费完成全部产业链。

验收：同环境文明仍可因早期随机分叉不同；遗迹突破同时产生政治和社会后果；历史存储与年份数近似按转折增长。

## M9-IC03 层级坐标和场景切换

对应：`M9-WP02`。新建 `SpatialFrameSubsystem`。

规范位置使用 FrameId、ParentFrameId、LocalTransform、Velocity、Epoch；解析到渲染坐标时沿父链组合并以玩家 Frame 为原点。跨 Frame 先计算世界状态，再变换到新父 Frame，保持速度和方向。UE Actor Transform 只是缓存，不回写宇宙规范位置。

验收：地面、轨道、飞船内部切换后相对位置连续；长时间航行无 FVector 精度累积；保存加载后 Frame 链一致。

## M9-IC04 Streaming 和 Detail on Demand

对应：`M9-WP02`、`M9-WP03`。

新建 `WorldPresentationCoordinator`，监听玩家位置和 Simulation LOD；决定加载星球代理、区域 HLOD、城市 Cell、NPC Actor。加载完成前显示代理层；卸载前先把表现状态提交给规范子系统；加载失败不改变人物规范存在性。

World Partition、PCG、Mesh Terrain 只消费 Planet/Region Recipe。实验功能资产可删除重建，不能保存人口、所有权或任务真相。

验收：快速往返不复制设施/NPC；Cell 未加载时世界时间和组织仍推进；画面代理切换不改变事实。

## M9-IC05 细节动作与降级汇总

对应：`M9-WP03`。

高细节交互由 `InteractionSequenceComponent` 执行步骤：取物、对准、入口/接触、消耗、放回。只有关键动画通知触发规范资源变化。液体按毫升、食物按质量/份额；容器状态属于物品系统。

人物离开 Hot 层时，未完成交互按最后已提交步骤汇总；消化队列、疾病暴露和下一生理关键点写入压缩状态；温/冷层只在关键点结算。

验收：喝一口瓶内减少一口；动画中断不凭空消耗剩余内容；远离十小时再回来消化和库存结果合理且无需十小时逐帧模拟。

# 跨卡实现顺序和接口冻结

1. M5 只能使用 M1 Commitment/Presence 公共 API，不直接改其 Map。
2. M6 复用 M5 Ticket/Travel 状态，不复制第二套旅行枚举。
3. M7 所有成长来源引用 Truth Event，不直接由击杀 Actor 加经验。
4. M8 组织行动通过服务与承诺执行，不直接改人物位置和库存。
5. M9 表现层只读规范模拟，通过明确命令提交交互结果。
6. 每跨过一个里程碑，冻结已公开 Save DTO 字段并提高 SchemaVersion。
