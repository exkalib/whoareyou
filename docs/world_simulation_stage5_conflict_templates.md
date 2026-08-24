# 阶段5模板：冲突与回退（延误/取消/改签/失联）

> 目标：当承诺与现实条件冲突时，不“硬跳”到目标状态，避免世界逻辑崩坏。

## 1) 冲突枚举与策略（可放 `WorldSimTypes.h`）

```cpp
UENUM(BlueprintType)
enum class ECommitmentConflictKind : uint8
{
    None UMETA(DisplayName="None"),
    Delayed UMETA(DisplayName="Delayed"),
    Missed UMETA(DisplayName="Missed"),
    Cancelled UMETA(DisplayName="Cancelled"),
    Rerouted UMETA(DisplayName="Rerouted"),
    LostContact UMETA(DisplayName="LostContact")
};

USTRUCT(BlueprintType)
struct FCommitmentFailure
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FGuid CommitmentId;

    UPROPERTY(BlueprintReadWrite)
    FGuid PersonId;

    UPROPERTY(BlueprintReadWrite)
    ECommitmentConflictKind Kind = ECommitmentConflictKind::None;

    UPROPERTY(BlueprintReadWrite)
    FString Reason;

    UPROPERTY(BlueprintReadWrite)
    int32 RetryCount = 0;

    UPROPERTY(BlueprintReadWrite)
    FDateTime NextTryAt;

    UPROPERTY(BlueprintReadWrite)
    FDateTime UpdatedAt;
};

USTRUCT(BlueprintType)
struct FTravelPolicy
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MaxRetry = 2;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float DelayMinutesBase = 30.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float DelayIncreaseStep = 45.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MissedChance = 0.15f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float LostChance = 0.05f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float CancelChance = 0.1f;
};
```

---

## 2) 冲突处理器（`CommitmentConflictResolver`）

### `CommitmentConflictResolver.h`
```cpp
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "WorldSimTypes.h"
#include "CommitmentConflictResolver.generated.h"

UCLASS()
class WORLDSIMDEMO_API UCommitmentConflictResolver : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="WorldSim|Commitment")
    bool ResolveOnConflict(const FCommitmentEvent& C, ECommitmentConflictKind ConflictKind,
        const FRegionSnapshot& FromSnapshot, const FRegionSnapshot& ToSnapshot, FCommitmentFailure& OutFailure);

    UFUNCTION(BlueprintCallable, Category="WorldSim|Commitment")
    FCommitmentFailure MakeFailure(const FCommitmentEvent& C, ECommitmentConflictKind Kind, const FString& Reason) const;

    UFUNCTION(BlueprintCallable, Category="WorldSim|Commitment")
    void PersistFailure(const FCommitmentFailure& Failure);

    UFUNCTION(BlueprintCallable, Category="WorldSim|Commitment")
    bool CanRetry(const FCommitmentFailure& Failure) const;

    UFUNCTION(BlueprintCallable, Category="WorldSim|Commitment")
    void OnRetrySchedule(FCommitmentFailure& Failure) const;

private:
    UPROPERTY(EditAnywhere)
    FTravelPolicy Policy;

    UPROPERTY()
    TMap<FGuid, FCommitmentFailure> Failures;
};
```

### `CommitmentConflictResolver.cpp`
```cpp
#include "CommitmentConflictResolver.h"
#include "Math/UnrealMathUtility.h"

FCommitmentFailure UCommitmentConflictResolver::MakeFailure(const FCommitmentEvent& C, ECommitmentConflictKind Kind, const FString& Reason) const
{
    FCommitmentFailure F;
    F.CommitmentId = C.CommitmentId;
    F.PersonId = C.PersonId;
    F.Kind = Kind;
    F.Reason = Reason;
    F.UpdatedAt = FDateTime::UtcNow();
    F.RetryCount = 0;
    F.NextTryAt = FDateTime::UtcNow();
    return F;
}

bool UCommitmentConflictResolver::ResolveOnConflict(const FCommitmentEvent& C, ECommitmentConflictKind ConflictKind,
    const FRegionSnapshot& FromSnapshot, const FRegionSnapshot& ToSnapshot, FCommitmentFailure& OutFailure)
{
    OutFailure = MakeFailure(C, ConflictKind, TEXT(""));

    // 1) 先看是否必须硬失败：硬承诺且任务关键性高
    const bool bHardImportant = C.bHardCommit && C.Type == ECommitmentType::Military;

    float failRoll = FMath::FRandRange(0.0f, 1.0f);

    if (ConflictKind == ECommitmentConflictKind::None)
    {
        return false;
    }

    if (bHardImportant && failRoll < Policy.CancelChance)
    {
        OutFailure.Kind = ECommitmentConflictKind::Cancelled;
        OutFailure.Reason = TEXT("hard_important_cancelled");
        OutFailure.RetryCount = Policy.MaxRetry;
        PersistFailure(OutFailure);
        return true;
    }

    // 2) 运输压力高 -> 优先延误
    if (FromSnapshot.TransportLoad > 1.2f || ToSnapshot.TransportLoad > 1.2f)
    {
        OutFailure.Kind = ECommitmentConflictKind::Delayed;
        OnRetrySchedule(OutFailure);
        PersistFailure(OutFailure);
        return true;
    }

    // 3) 丢失联系概率（航线异常）
    if (failRoll < Policy.LostChance)
    {
        OutFailure.Kind = ECommitmentConflictKind::LostContact;
        OutFailure.Reason = TEXT("telemetry_lost");
        OutFailure.RetryCount = 0;
        PersistFailure(OutFailure);
        return true;
    }

    // 4) 时间窗错过：是否重试
    if (failRoll < Policy.MissedChance && OutFailure.RetryCount < Policy.MaxRetry)
    {
        OutFailure.Kind = ECommitmentConflictKind::Missed;
        OnRetrySchedule(OutFailure);
        PersistFailure(OutFailure);
        return true;
    }

    // 5) 默认改签到下一班/下一窗口
    OutFailure.Kind = ECommitmentConflictKind::Rerouted;
    OnRetrySchedule(OutFailure);
    PersistFailure(OutFailure);
    return true;
}

void UCommitmentConflictResolver::OnRetrySchedule(FCommitmentFailure& Failure) const
{
    const int32 Next = FMath::Clamp(Failure.RetryCount, 0, 10);
    const float Minutes = Policy.DelayMinutesBase + Policy.DelayIncreaseStep * Next;

    Failure.NextTryAt = FDateTime::UtcNow() + FTimespan::FromMinutes(Minutes);
    Failure.UpdatedAt = FDateTime::UtcNow();
    Failure.RetryCount += 1;
}

bool UCommitmentConflictResolver::CanRetry(const FCommitmentFailure& Failure) const
{
    return Failure.RetryCount < Policy.MaxRetry && FDateTime::UtcNow() >= Failure.NextTryAt;
}

void UCommitmentConflictResolver::PersistFailure(const FCommitmentFailure& Failure)
{
    Failures.Add(Failure.CommitmentId, Failure);
}
```

---

## 3) 认知层（新闻/传闻分离）最小实现

### `KnowledgeSubsystem.h`
```cpp
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "WorldSimTypes.h"
#include "KnowledgeSubsystem.generated.h"

UCLASS()
class WORLDSIMDEMO_API UKnowledgeSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="WorldSim|Knowledge")
    void AddRumorOrNews(const FPlayerKnowledgeItem& Item);

    UFUNCTION(BlueprintCallable, Category="WorldSim|Knowledge")
    TArray<FPlayerKnowledgeItem> QueryPersonKnowledge(FGuid PersonId) const;

    UFUNCTION(BlueprintCallable, Category="WorldSim|Knowledge")
    void MarkVerified(const FGuid& FactId, bool bIsTrue);

private:
    UPROPERTY()
    TArray<FPlayerKnowledgeItem> Feed;
};
```

### `KnowledgeSubsystem.cpp`
```cpp
#include "KnowledgeSubsystem.h"

void UKnowledgeSubsystem::AddRumorOrNews(const FPlayerKnowledgeItem& Item)
{
    Feed.Add(Item);
}

TArray<FPlayerKnowledgeItem> UKnowledgeSubsystem::QueryPersonKnowledge(FGuid PersonId) const
{
    TArray<FPlayerKnowledgeItem> Out;
    FString Tag = PersonId.ToString();
    for (const FPlayerKnowledgeItem& K : Feed)
    {
        if (K.Source.Contains(Tag))
        {
            Out.Add(K);
        }
    }
    return Out;
}

void UKnowledgeSubsystem::MarkVerified(const FGuid& FactId, bool bIsTrue)
{
    for (FPlayerKnowledgeItem& K : Feed)
    {
        if (K.FactId == FactId)
        {
            K.bVerified = bIsTrue;
            K.Confidence = bIsTrue ? EKnowledgeConfidence::Verified : EKnowledgeConfidence::Low;
            return;
        }
    }
}
```

---

## 4) 将冲突处理接到调度器（阶段2模板的衔接）

在你的 `WorldSimSchedulerSubsystem::EvaluateOneCommitment` 中，原有的失败/冲突分支改为：

```cpp
UCommitmentConflictResolver* Resolver = GetGameInstance() ? GetGameInstance()->GetSubsystem<UCommitmentConflictResolver>() : nullptr;
if (!Resolver)
{
    // 兼容旧逻辑
}

// 示例：检测到冲突
FCommitmentFailure Failure;
if (!Resolver->ResolveOnConflict(C, ECommitmentConflictKind::None, RegionSub->GetSnapshot(C.FromLocationId), RegionSub->GetSnapshot(C.ToLocationId), Failure))
{
    // 正常执行
}
else
{
    // 按处理类型写入事件和认知
    switch (Failure.Kind)
    {
        case ECommitmentConflictKind::Delayed:
            EmitEvent(TimeSub, TruthSub, C.CommitmentId, C.PersonId, TEXT("Delayed"), C.FromLocationId, Failure.Reason);
            break;
        case ECommitmentConflictKind::Missed:
            EmitEvent(TimeSub, TruthSub, C.CommitmentId, C.PersonId, TEXT("MissedDeparture"), C.FromLocationId, Failure.Reason);
            break;
        case ECommitmentConflictKind::Cancelled:
            EmitEvent(TimeSub, TruthSub, C.CommitmentId, C.PersonId, TEXT("Cancelled"), C.FromLocationId, Failure.Reason);
            break;
        case ECommitmentConflictKind::Rerouted:
            EmitEvent(TimeSub, TruthSub, C.CommitmentId, C.PersonId, TEXT("Rerouted"), C.ToLocationId, Failure.Reason);
            break;
        case ECommitmentConflictKind::LostContact:
            EmitEvent(TimeSub, TruthSub, C.CommitmentId, C.PersonId, TEXT("LostContact"), C.FromLocationId, Failure.Reason);
            break;
        default:
            break;
    }

    // 同时给玩家记一条低可信消息（可选）
    UKnowledgeSubsystem* K = GetGameInstance() ? GetGameInstance()->GetSubsystem<UKnowledgeSubsystem>() : nullptr;
    if (K && Failure.Kind != ECommitmentConflictKind::None)
    {
        FPlayerKnowledgeItem Ki;
        Ki.FactId = Failure.CommitmentId;
        Ki.Source = TEXT("TravelScheduler");
        Ki.Content = Failure.Reason;
        Ki.Confidence = EKnowledgeConfidence::Low;
        Ki.ObservedAt = FDateTime::UtcNow();
        K->AddRumorOrNews(Ki);
    }
}
```

---

## 5) 冲突恢复（定时器）

给调度器加一段重试扫描（例如每小时）：

```cpp
void UWorldSimSchedulerSubsystem::ProcessRetries(float DeltaSeconds)
{
    UCommitmentConflictResolver* Resolver = GetGameInstance()->GetSubsystem<UCommitmentConflictResolver>();
    UCommitmentSubsystem* CommitmentSub = GetWorld()->GetSubsystem<UCommitmentSubsystem>();
    if (!Resolver || !CommitmentSub) return;

    // 这里只示意：你可把 Failures 公开接口提供迭代
    // 当 CanRetry = true 时，把 Commitment 的 Earliest/Latest 往后推并重新放回待处理队列
}
```

---

## 6) 阶段5验收动作

1. 同一个人同时间给两个硬承诺（重叠）；一个应因 PresenceConflict 标记为 `Rerouted/Missed`。
2. 观察 `TruthLedger` 出现 `Delayed -> Arrived/Cancelled` 而非瞬移。
3. 让 `TransportLoad` 拉高，承诺应进入 `Delayed`；重试后再执行。
4. 触发 `LostContact` 后：在一段时间内其状态变成 `Unknown`，随后可由外部事件修复。
5. `KnowledgeSubsystem` 显示同一事件的“有来源但低可信”与“真相层 Verified”差异。

---

> 这部分是你世界“可信”感最关键的开关：真实世界走真相日志，玩家信息层有可信度与延迟。
