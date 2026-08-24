# 阶段2联调模板：承诺执行链路（可复现、不穿帮）

## 1) 先决条件
你已经有这些类（来自阶段1）：
- `UCommitmentSubsystem`（承诺存取）
- `UPresenceSubsystem`（存在区间锁）
- `UTruthLedgerSubsystem`（真相日志）
- `URegionSnapshotSubsystem`（区域快照）

本模板的目标：
1. 对话里生成承诺
2. 定时器扫描承诺
3. 生成 `PresenceInterval`
4. 按规则写入 `WorldEvent`
5. 失败/延误/取消可回退

---

## 2) 数据建议补充（可直接合并到 `WorldSimTypes.h`）
```cpp
UENUM(BlueprintType)
enum class ECommitmentLifecycle : uint8
{
    Pending,
    Departing,
    InTransit,
    InOperation,
    Returning,
    Settled,
    Cancelled,
    Failed
};

USTRUCT(BlueprintType)
struct FCommitmentRuntime
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FGuid CommitmentId;

    UPROPERTY(BlueprintReadWrite)
    ECommitmentLifecycle Stage = ECommitmentLifecycle::Pending;

    UPROPERTY(BlueprintReadWrite)
    FDateTime StageUpdatedAt = FDateTime::Now();

    UPROPERTY(BlueprintReadWrite)
    float RiskMultiplier = 1.0f;

    UPROPERTY(BlueprintReadWrite)
    FString LastFailureReason;
};

USTRUCT(BlueprintType)
struct FRouteInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    int32 RouteId = INDEX_NONE;

    UPROPERTY(BlueprintReadWrite)
    float TravelMinutes = 120.0f;

    UPROPERTY(BlueprintReadWrite)
    float Cost = 100.0f;

    UPROPERTY(BlueprintReadWrite)
    int32 Capacity = 100;

    UPROPERTY(BlueprintReadWrite)
    float CongestionMultiplier = 1.0f;

    UPROPERTY(BlueprintReadWrite)
    bool bReachable = true;
};
```

建议新增 `TMap<FGuid, FCommitmentRuntime> CommitmentRuntime` 到 `UCommitmentSubsystem`。

---

## 3) `WorldSimSchedulerSubsystem`（执行器，阶段2最小骨架）
### `WorldSimSchedulerSubsystem.h`
```cpp
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WorldSimTypes.h"
#include "WorldSimSchedulerSubsystem.generated.h"

UCLASS()
class WORLDSIMDEMO_API UWorldSimSchedulerSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="WorldSim|Scheduler")
    void TickScheduler(float DeltaSeconds);

    UFUNCTION(BlueprintCallable, Category="WorldSim|Scheduler")
    void ForceProcessCommitment(FGuid CommitmentId);

private:
    void ProcessDueCommitments(float DeltaSeconds);
    void EvaluateOneCommitment(FGuid CommitmentId, const FCommitmentEvent& C, const FDateTime& Now);
    bool BuildRoute(const FCommitmentEvent& C, FRouteInfo& OutRoute) const;
    bool ConsumeResourcesForCommitment(FGuid PersonId, const FCommitmentEvent& C, const FRouteInfo& Route);
    void EmitEvent(UWorldTimeSubsystem* TimeSub, UTruthLedgerSubsystem* TruthSub,
                   FGuid CommitmentId, FGuid PersonId, const FString& Type,
                   int32 LocationId, const FString& JsonPayload = TEXT(""));
    bool TrySetPresenceLocked(UWorld* World, const FCommitmentEvent& C, const FDateTime& Departure, const FDateTime& Arrival);
    void SettleOutcome(const FCommitmentEvent& C, FDateTime At, UTruthLedgerSubsystem* TruthSub);

    float AccumulatedTime = 0.0f;
};
```

### `WorldSimSchedulerSubsystem.cpp`
```cpp
#include "WorldSimSchedulerSubsystem.h"
#include "WorldTimeSubsystem.h"
#include "CommitmentSubsystem.h"
#include "PresenceSubsystem.h"
#include "TruthLedgerSubsystem.h"
#include "RegionSnapshotSubsystem.h"

void UWorldSimSchedulerSubsystem::TickScheduler(float DeltaSeconds)
{
    AccumulatedTime += DeltaSeconds;
    if (AccumulatedTime >= 1.0f)
    {
        ProcessDueCommitments(AccumulatedTime);
        AccumulatedTime = 0.0f;
    }
}

void UWorldSimSchedulerSubsystem::ProcessDueCommitments(float DeltaSeconds)
{
    UWorldTimeSubsystem* TimeSub = GetWorld()->GetSubsystem<UWorldTimeSubsystem>();
    UCommitmentSubsystem* CommitmentSub = GetWorld()->GetSubsystem<UCommitmentSubsystem>();
    UTruthLedgerSubsystem* TruthSub = GetWorld()->GetSubsystem<UTruthLedgerSubsystem>();
    if (!TimeSub || !CommitmentSub || !TruthSub) return;

    const FDateTime Now = TimeSub->GetNow() + FTimespan::FromSeconds(DeltaSeconds);
    TimeSub->AdvanceWorldTime(DeltaSeconds);

    const TArray<FGuid> Due = CommitmentSub->GetDueCommitments(Now, 24.0f);
    for (const FGuid& CommitmentId : Due)
    {
        EvaluateOneCommitment(CommitmentId, CommitmentSub->GetCommitmentsMap().FindChecked(CommitmentId), Now);
    }
}

void UWorldSimSchedulerSubsystem::ForceProcessCommitment(FGuid CommitmentId)
{
    UCommitmentSubsystem* CommitmentSub = GetWorld()->GetSubsystem<UCommitmentSubsystem>();
    UWorldTimeSubsystem* TimeSub = GetWorld()->GetSubsystem<UWorldTimeSubsystem>();
    if (!CommitmentSub || !TimeSub) return;

    if (const FCommitmentEvent* C = CommitmentSub->GetCommitmentsMap().Find(CommitmentId))
    {
        EvaluateOneCommitment(CommitmentId, *C, TimeSub->GetNow());
    }
}

void UWorldSimSchedulerSubsystem::EvaluateOneCommitment(FGuid CommitmentId, const FCommitmentEvent& C, const FDateTime& Now)
{
    UWorldTimeSubsystem* TimeSub = GetWorld()->GetSubsystem<UWorldTimeSubsystem>();
    UCommitmentSubsystem* CommitmentSub = GetWorld()->GetSubsystem<UCommitmentSubsystem>();
    UPresenceSubsystem* PresenceSub = GetWorld()->GetSubsystem<UPresenceSubsystem>();
    UTruthLedgerSubsystem* TruthSub = GetWorld()->GetSubsystem<UTruthLedgerSubsystem>();
    URegionSnapshotSubsystem* RegionSub = GetWorld()->GetSubsystem<URegionSnapshotSubsystem>();

    if (!TimeSub || !CommitmentSub || !PresenceSub || !TruthSub || !RegionSub) return;

    // 1) 时间窗口判断
    if (Now < C.EarliestStart || Now > C.LatestStart)
    {
        return;
    }

    // 2) 路径规划
    FRouteInfo Route;
    if (!BuildRoute(C, Route) || !Route.bReachable)
    {
        CommitmentSub->UpdateCommitmentState(CommitmentId, TEXT("failed"), TEXT("No route"));
        EmitEvent(TimeSub, TruthSub, CommitmentId, C.PersonId, TEXT("Cancelled"), C.FromLocationId, TEXT("{ \"reason\": \"no_route\" }"));
        return;
    }

    // 3) 资源可用性（可改：货币/通勤票/许可）
    if (!ConsumeResourcesForCommitment(C.PersonId, C, Route))
    {
        CommitmentSub->UpdateCommitmentState(CommitmentId, TEXT("delayed"), TEXT("No resources"));
        EmitEvent(TimeSub, TruthSub, CommitmentId, C.PersonId, TEXT("Delayed"), C.FromLocationId, TEXT("{ \"reason\": \"insufficient_resources\" }"));
        return;
    }

    // 4) 存在性锁（防双处）
    FDateTime DepartAt = Now;
    FDateTime ArriveAt = Now + FTimespan::FromMinutes(Route.TravelMinutes * Route.CongestionMultiplier);

    if (!TrySetPresenceLocked(GetWorld(), C, DepartAt, ArriveAt))
    {
        // 说明此人正被其他承诺占用
        EmitEvent(TimeSub, TruthSub, CommitmentId, C.PersonId, TEXT("Conflict"), C.FromLocationId, TEXT("{ \"reason\": \"presence_conflict\" }"));
        CommitmentSub->UpdateCommitmentState(CommitmentId, TEXT("failed"), TEXT("Presence conflict"));
        return;
    }

    CommitmentSub->UpdateCommitmentState(CommitmentId, TEXT("departing"), TEXT(""));
    EmitEvent(TimeSub, TruthSub, CommitmentId, C.PersonId, TEXT("Departed"), C.FromLocationId, TEXT("{ \"route\": "") + FString::FromInt(C.RouteId) + TEXT("\" }"));

    // 5) 到达事件（简化：同一tick内写入到达，后续可按时间推进）
    EmitEvent(TimeSub, TruthSub, CommitmentId, C.PersonId, TEXT("Arrived"), C.ToLocationId, TEXT("{ \"stage\": \"arrived\" }"));

    // 6) 过程中可插入冲突结算：战斗、意外、堵车导致延误
    SettleOutcome(C, ArriveAt, TruthSub);
    PresenceSub->ReleasePresence(C.PersonId, ArriveAt);

    // 7) 区域快照回写
    RegionSub->ApplyMicroDelta(C.FromLocationId, -1, RouteIdToTrafficDelta(C.RouteId));
    RegionSub->ApplyMicroDelta(C.ToLocationId, +1, RouteIdToTrafficDelta(C.RouteId));

    CommitmentSub->UpdateCommitmentState(CommitmentId, TEXT("settled"), TEXT(""));
    EmitEvent(TimeSub, TruthSub, CommitmentId, C.PersonId, TEXT("Returned"), C.ToLocationId, TEXT("{ \"result\": \"arrived\" }"));
}

bool UWorldSimSchedulerSubsystem::BuildRoute(const FCommitmentEvent& C, FRouteInfo& OutRoute) const
{
    // 暂时按给定 RouteId 直给固定参数
    OutRoute.RouteId = C.RouteId;
    OutRoute.TravelMinutes = 60.0f;
    OutRoute.Cost = 100.0f;
    OutRoute.Capacity = 20;
    OutRoute.CongestionMultiplier = 1.0f;
    OutRoute.bReachable = (C.FromLocationId != C.ToLocationId);
    return OutRoute.bReachable;
}

bool UWorldSimSchedulerSubsystem::ConsumeResourcesForCommitment(FGuid PersonId, const FCommitmentEvent& C, const FRouteInfo& Route)
{
    // 最小可用规则：先按可达 + 承诺类型
    if (!C.bHardCommit)
    {
        return true;
    }
    return C.CostBudget <= 0.0f || Route.Cost <= C.CostBudget;
}

bool UWorldSimSchedulerSubsystem::TrySetPresenceLocked(UWorld* World, const FCommitmentEvent& C, const FDateTime& Departure, const FDateTime& Arrival)
{
    UPresenceSubsystem* PresenceSub = World ? World->GetSubsystem<UPresenceSubsystem>() : nullptr;
    if (!PresenceSub)
    {
        return false;
    }

    FPresenceInterval Interval;
    Interval.PersonId = C.PersonId;
    Interval.StartTime = Departure;
    Interval.EndTime = Arrival + FTimespan::FromHours(8); // 简化：任务窗口
    Interval.LocationId = C.ToLocationId;
    Interval.CommitmentId = C.CommitmentId;
    Interval.StateTag = EPersonActivityState::InOperation;

    return PresenceSub->LockPresence(Interval);
}

void UWorldSimSchedulerSubsystem::SettleOutcome(const FCommitmentEvent& C, FDateTime At, UTruthLedgerSubsystem* TruthSub)
{
    if (!TruthSub) return;

    if (C.Type == ECommitmentType::Military)
    {
        // 示例：低概率死亡
        const bool bDead = (FMath::RandRange(0, 1000) == 0);
        if (bDead)
        {
            FWorldEvent Evt;
            Evt.EventType = TEXT("Dead");
            Evt.EventId = FGuid::NewGuid();
            Evt.PersonId = C.PersonId;
            Evt.SourceCommitmentId = C.CommitmentId;
            Evt.EventTime = At;
            Evt.LocationId = C.ToLocationId;
            TruthSub->AppendEvent(Evt);
            return;
        }
    }

    FWorldEvent Evt;
    Evt.EventType = TEXT("Survived");
    Evt.EventId = FGuid::NewGuid();
    Evt.PersonId = C.PersonId;
    Evt.SourceCommitmentId = C.CommitmentId;
    Evt.EventTime = At;
    Evt.LocationId = C.ToLocationId;
    TruthSub->AppendEvent(Evt);
}

void UWorldSimSchedulerSubsystem::EmitEvent(UWorldTimeSubsystem* TimeSub, UTruthLedgerSubsystem* TruthSub,
    FGuid CommitmentId, FGuid PersonId, const FString& Type,
    int32 LocationId, const FString& JsonPayload)
{
    if (!TimeSub || !TruthSub) return;

    FWorldEvent E;
    E.EventId = FGuid::NewGuid();
    E.PersonId = PersonId;
    E.SourceCommitmentId = CommitmentId;
    E.EventType = Type;
    E.EventTime = TimeSub->GetNow();
    E.LocationId = LocationId;
    E.JsonPayload = JsonPayload;
    TruthSub->AppendEvent(E);
}

inline int32 RouteIdToTrafficDelta(int32 RouteId)
{
    return FMath::Max(1, RouteId % 3 + 1);
}
```

> 说明：这段里我把“到达/执行/返回”简化同一tick处理。后续建议加一个 `Pending/Active/Completable` 的时间戳驱动推进器。

---

## 4) 补充：对话触发承诺（软承诺/硬承诺分离）

- `soft commitment`：不锁区域，不强执行，作为认知层消息。
- `hard commitment`：有时间窗和承诺后果，必须进 `CommitmentSubsystem`。

### `ConversationCommitmentBridge.h/.cpp`
```cpp
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "WorldSimTypes.h"
#include "ConversationCommitmentBridge.generated.h"

UCLASS()
class WORLDSIMDEMO_API UConversationCommitmentBridge : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="WorldSim|Dialogue")
    FGuid OnNpcSaysHardEvent(FGuid SpeakerId, ECommitmentType Type, FDateTime EarliestStart, FDateTime LatestStart,
                            int32 FromRegionId, int32 ToRegionId, int32 RouteId, float CostBudget = 0.0f);

    UFUNCTION(BlueprintCallable, Category="WorldSim|Dialogue")
    void OnNpcSaysSoftEvent(FGuid SpeakerId, const FString& Message, bool bReliable);
};
```

```cpp
#include "ConversationCommitmentBridge.h"
#include "CommitmentSubsystem.h"
#include "KnowledgeSubsystem.h"

FGuid UConversationCommitmentBridge::OnNpcSaysHardEvent(FGuid SpeakerId, ECommitmentType Type, FDateTime EarliestStart, FDateTime LatestStart,
    int32 FromRegionId, int32 ToRegionId, int32 RouteId, float CostBudget)
{
    UCommitmentSubsystem* CommitmentSub = GetGameInstance()->GetSubsystem<UCommitmentSubsystem>();
    if (!CommitmentSub) return FGuid();

    FCommitmentEvent C;
    C.CommitmentId = FGuid::NewGuid();
    C.PersonId = SpeakerId;
    C.Type = Type;
    C.EarliestStart = EarliestStart;
    C.LatestStart = LatestStart;
    C.ExpectedEnd = LatestStart + FTimespan::FromHours(8);
    C.FromLocationId = FromRegionId;
    C.ToLocationId = ToRegionId;
    C.RouteId = RouteId;
    C.CostBudget = CostBudget;
    C.bHardCommit = true;
    C.bCancelable = false;

    return CommitmentSub->CreateCommitment(C);
}

void UConversationCommitmentBridge::OnNpcSaysSoftEvent(FGuid SpeakerId, const FString& Message, bool bReliable)
{
    // 这里把“他可能要去前线/明天回来”先打到认知层，不直接强约束
    if (UKnowledgeSubsystem* Knowledge = GetGameInstance()->GetSubsystem<UKnowledgeSubsystem>())
    {
        FPlayerKnowledgeItem Item;
        Item.FactId = FGuid::NewGuid();
        Item.Source = TEXT("NPC:") + SpeakerId.ToString();
        Item.Content = Message;
        Item.Confidence = bReliable ? EKnowledgeConfidence::Medium : EKnowledgeConfidence::Low;
        Item.ObservedAt = FDateTime::UtcNow();
        Item.bVerified = false;
        Knowledge->AddRumorOrNews(Item);
    }
}
```

> 说明：如果某句话只是“可能/也许/听说”，走 `OnNpcSaysSoftEvent`。

---

## 5) 阶段2最小联调测试清单（建议）

1. 创建一个NPC并手工设置承诺：今天 10:00-10:30 去旅游（`bHardCommit=true`）。
2. 模拟时间推进到 10:15。
3. 调度器 `TickScheduler` -> 生成 `Departed/Arrived/Survived`。
4. 查 `TruthLedger`：最后状态为 `Survived` 且位置是目标区域。
5. 在该NPC尚在任务区间再次尝试加载到另一区域 -> `CanAppear=false`。
6. 再尝试在 3 秒后给同人再建硬承诺 -> `PresenceConflict`/`Failed`。

---

## 6) 常见坑（防穿帮）

1. `GetDueCommitments` 查询时间窗里不要只查 `EarliestStart`，要加容差。
2. “软承诺”绝对不要更新 `PresenceInterval`。
3. `CommitmentId` 和 `PersonId` 全程以 GUID 为主键，避免按临时显示索引。
4. 一个区域可见池必须以 `CanAppear + PresenceInterval` 过滤。
5. 写真相前后都记录 `EventTime` 与 `LocationId`。
