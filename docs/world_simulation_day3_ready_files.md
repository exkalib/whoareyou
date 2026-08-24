# Day3 可直接贴入 UE5 的代码（承诺 + 存在性）

Day2 之后，今天补齐“是否真的能在某地存在”的硬约束。  
你会拿到 3 组文件：
1) `CommitmentSubsystem`  
2) `PresenceSubsystem`  
3) `WorldSimBlueprintFunctionLibrary` 的存在查询入口

> 目标是可编译且能跑通最小一致性：**承诺->存在锁->查询可出现**。

---

## 1. `Source/WorldSimDemo/Public/PresenceSubsystem.h`

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WorldSimCoreTypes.h"
#include "PresenceSubsystem.generated.h"

UCLASS()
class WORLDSIMDEMO_API UPresenceSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "WorldSim|Presence")
    bool TryReservePresence(const FPresenceInterval& Interval, FString* RejectReason = nullptr);

    UFUNCTION(BlueprintCallable, Category = "WorldSim|Presence")
    void ReleasePresence(FGuid PersonId, FDateTime At);

    UFUNCTION(BlueprintPure, Category = "WorldSim|Presence")
    bool CanAppearAt(FGuid PersonId, int32 RegionId, FDateTime At) const;

    UFUNCTION(BlueprintPure, Category = "WorldSim|Presence")
    bool IsPersonAt(FGuid PersonId, int32 RegionId, FDateTime At) const;

    UFUNCTION(BlueprintPure, Category = "WorldSim|Presence")
    TOptional<FPresenceInterval> GetPresence(FGuid PersonId) const;

    UFUNCTION(BlueprintPure, Category = "WorldSim|Presence")
    TArray<FGuid> GetPeopleAt(FDateTime At, int32 RegionId) const;

    UFUNCTION(BlueprintCallable, Category = "WorldSim|Presence")
    void DebugSweepExpired(FDateTime Now);

private:
    UPROPERTY()
    TMap<FGuid, FPresenceInterval> ActiveIntervals;
};
```

## `Source/WorldSimDemo/Private/PresenceSubsystem.cpp`

```cpp
#include "PresenceSubsystem.h"

static bool HasOverlap(const FPresenceInterval& A, const FPresenceInterval& B)
{
    // 半开区间：[Start, End)
    return A.StartTime < B.EndTime && B.StartTime < A.EndTime;
}

bool UPresenceSubsystem::TryReservePresence(const FPresenceInterval& Interval, FString* RejectReason)
{
    if (!Interval.PersonId.IsValid())
    {
        if (RejectReason) *RejectReason = TEXT("PersonId 无效");
        return false;
    }

    if (Interval.StartTime >= Interval.EndTime)
    {
        if (RejectReason) *RejectReason = TEXT("时间区间非法（Start>=End）");
        return false;
    }

    if (const FPresenceInterval* Existing = ActiveIntervals.Find(Interval.PersonId))
    {
        if (HasOverlap(*Existing, Interval))
        {
            if (RejectReason) *RejectReason = TEXT("与已有存在区间冲突（同一人重叠在场）");
            return false;
        }
    }

    ActiveIntervals.Add(Interval.PersonId, Interval);
    return true;
}

void UPresenceSubsystem::ReleasePresence(FGuid PersonId, FDateTime At)
{
    if (FPresenceInterval* Existing = ActiveIntervals.Find(PersonId))
    {
        if (At < Existing->EndTime)
        {
            Existing->EndTime = At;
        }
    }
}

bool UPresenceSubsystem::CanAppearAt(FGuid PersonId, int32 RegionId, FDateTime At) const
{
    if (const FPresenceInterval* I = ActiveIntervals.Find(PersonId))
    {
        return I->RegionId == RegionId && I->StartTime <= At && At < I->EndTime;
    }

    // 无记录表示未限制，可按需让上层系统继续处理。
    return true;
}

bool UPresenceSubsystem::IsPersonAt(FGuid PersonId, int32 RegionId, FDateTime At) const
{
    return CanAppearAt(PersonId, RegionId, At);
}

TOptional<FPresenceInterval> UPresenceSubsystem::GetPresence(FGuid PersonId) const
{
    if (const FPresenceInterval* I = ActiveIntervals.Find(PersonId))
    {
        return TOptional<FPresenceInterval>(*I);
    }
    return TOptional<FPresenceInterval>();
}

TArray<FGuid> UPresenceSubsystem::GetPeopleAt(FDateTime At, int32 RegionId) const
{
    TArray<FGuid> Out;
    for (const auto& Pair : ActiveIntervals)
    {
        const FPresenceInterval& I = Pair.Value;
        if (I.RegionId == RegionId && I.StartTime <= At && At < I.EndTime)
        {
            Out.Add(Pair.Key);
        }
    }
    return Out;
}

void UPresenceSubsystem::DebugSweepExpired(FDateTime Now)
{
    TArray<FGuid> ToRemove;
    for (const auto& Pair : ActiveIntervals)
    {
        if (Pair.Value.EndTime <= Now)
        {
            ToRemove.Add(Pair.Key);
        }
    }

    for (FGuid Id : ToRemove)
    {
        ActiveIntervals.Remove(Id);
    }
}
```

---

## 2. `Source/WorldSimDemo/Public/CommitmentSubsystem.h`

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WorldSimCoreTypes.h"
#include "CommitmentSubsystem.generated.h"

UCLASS()
class WORLDSIMDEMO_API UCommitmentSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "WorldSim|Commitment")
    FGuid CreateCommitment(const FCommitmentRecord& InCommitment);

    UFUNCTION(BlueprintCallable, Category = "WorldSim|Commitment")
    bool CancelCommitment(FGuid CommitmentId, const FString& Reason = TEXT("Cancelled"));

    UFUNCTION(BlueprintCallable, Category = "WorldSim|Commitment")
    bool UpdateCommitmentState(FGuid CommitmentId, ECommitmentState NewState, const FString& Reason = TEXT(""));

    UFUNCTION(BlueprintCallable, Category = "WorldSim|Commitment")
    bool BindPresence(FGuid CommitmentId, const FPresenceInterval& Interval, FString* RejectReason = nullptr);

    UFUNCTION(BlueprintCallable, Category = "WorldSim|Commitment")
    TArray<FGuid> GetCommitmentsForPerson(FGuid PersonId) const;

    UFUNCTION(BlueprintCallable, Category = "WorldSim|Commitment")
    TArray<FGuid> GetDueCommitments(FDateTime Now, float LookAheadHours = 1.0f) const;

private:
    UPROPERTY()
    TMap<FGuid, FCommitmentRecord> Commitments;

    UPROPERTY()
    TMap<FGuid, TArray<FGuid>> PersonCommitments;
};
```

## `Source/WorldSimDemo/Private/CommitmentSubsystem.cpp`

```cpp
#include "CommitmentSubsystem.h"

FGuid UCommitmentSubsystem::CreateCommitment(const FCommitmentRecord& InCommitment)
{
    FCommitmentRecord Copy = InCommitment;
    if (!Copy.CommitmentId.IsValid())
    {
        Copy.CommitmentId = FGuid::NewGuid();
    }

    if (Copy.EarliestStart > Copy.LatestStart)
    {
        Copy.LatestStart = Copy.EarliestStart;
    }

    Commitments.Add(Copy.CommitmentId, Copy);
    PersonCommitments.FindOrAdd(Copy.PersonId).AddUnique(Copy.CommitmentId);
    return Copy.CommitmentId;
}

bool UCommitmentSubsystem::CancelCommitment(FGuid CommitmentId, const FString& Reason)
{
    if (!Commitments.Contains(CommitmentId))
    {
        return false;
    }

    FCommitmentRecord& C = Commitments[CommitmentId];
    C.State = ECommitmentState::Cancelled;
    C.bExecuted = true;
    return true;
}

bool UCommitmentSubsystem::UpdateCommitmentState(FGuid CommitmentId, ECommitmentState NewState, const FString& Reason)
{
    FCommitmentRecord* C = Commitments.Find(CommitmentId);
    if (!C)
    {
        return false;
    }

    C->State = NewState;
    if (NewState == ECommitmentState::Completed || NewState == ECommitmentState::Cancelled || NewState == ECommitmentState::Failed)
    {
        C->bExecuted = true;
    }
    return true;
}

bool UCommitmentSubsystem::BindPresence(FGuid CommitmentId, const FPresenceInterval& Interval, FString* RejectReason)
{
    FCommitmentRecord* C = Commitments.Find(CommitmentId);
    if (!C)
    {
        if (RejectReason) *RejectReason = TEXT("未找到承诺");
        return false;
    }

    if (Interval.PersonId != C->PersonId)
    {
        if (RejectReason) *RejectReason = TEXT("PresencePerson 与承诺人不一致");
        return false;
    }

    C->State = ECommitmentState::Locked;
    return true;
}

TArray<FGuid> UCommitmentSubsystem::GetCommitmentsForPerson(FGuid PersonId) const
{
    if (const TArray<FGuid>* L = PersonCommitments.Find(PersonId))
    {
        return *L;
    }
    return {};
}

TArray<FGuid> UCommitmentSubsystem::GetDueCommitments(FDateTime Now, float LookAheadHours) const
{
    TArray<FGuid> Out;
    const FTimespan Window = FTimespan::FromHours(LookAheadHours);

    for (const auto& Pair : Commitments)
    {
        const FCommitmentRecord& C = Pair.Value;
        if (C.State == ECommitmentState::Completed || C.State == ECommitmentState::Cancelled || C.State == ECommitmentState::Failed)
        {
            continue;
        }

        if (!C.bExecuted && C.EarliestStart <= Now && Now <= C.LatestStart + Window)
        {
            Out.Add(Pair.Key);
        }
    }
    return Out;
}
```

---

## 3. `Source/WorldSimDemo/Public/WorldSimBlueprintFunctionLibrary.h`

把 Day1 的库继续补上：

```cpp
// 在同一文件内继续追加
UFUNCTION(BlueprintPure, Category = "WorldSim|API", meta = (WorldContext = "WorldContextObject"))
static bool BS_CanPersonExist(const UObject* WorldContextObject, FGuid PersonId, int32 RegionId, FDateTime At);
```

## `Source/WorldSimDemo/Private/WorldSimBlueprintFunctionLibrary.cpp`

```cpp
#include "TruthLedgerSubsystem.h"
#include "PresenceSubsystem.h"
#include "WorldSimBlueprintFunctionLibrary.h"

bool UWorldSimBlueprintFunctionLibrary::BS_CanPersonExist(const UObject* WorldContextObject, FGuid PersonId, int32 RegionId, FDateTime At)
{
    if (!WorldContextObject || !GEngine)
    {
        return true;
    }

    UWorld* World = GEngine->GetWorldFromContextObjectChecked(WorldContextObject);
    if (!World)
    {
        return true;
    }

    if (auto* TruthSub = World->GetSubsystem<UTruthLedgerSubsystem>())
    {
        EExistenceState S;
        FWorldEvent Evt;
        if (TruthSub->ResolveLatestState(PersonId, S, Evt) && S == EExistenceState::Dead)
        {
            return false;
        }
    }

    if (auto* PresenceSub = World->GetSubsystem<UPresenceSubsystem>())
    {
        return PresenceSub->CanAppearAt(PersonId, RegionId, At);
    }

    return true;
}
```

---

## 4. （建议）把 Commit -> Presence 串成一条最小规则（你在系统调用里加）

当创建承诺并准备执行时，直接做两个动作：
1) `CommitmentSubsystem::CreateCommitment`  
2) 用承诺生成 `FPresenceInterval`，调用 `PresenceSubsystem::TryReservePresence`  
这样就能保证同一时刻同一人不会在两个地方“同时活着”。

示例逻辑：
```cpp
FGuid Id = CommitmentSub->CreateCommitment(Commit);
FPresenceInterval I;
I.PersonId = Commit.PersonId;
I.StartTime = Commit.EarliestStart;
I.EndTime = Commit.ExpectedEnd;
I.RegionId = Commit.FromLocationId;
I.CommitmentId = Id;

FString Reject;
if (!PresenceSub->TryReservePresence(I, &Reject))
{
    // 若冲突，回滚承诺（或改成待处理）
}
```

---

## 5. Day3 验收（必须）

1. 两段同一人重叠区间的 `TryReservePresence` 其中一段失败（返回 false）。
2. 一个 `Commitment` 能找到并进入 `GetDueCommitments`（在时间窗口内未完成）。
3. `BS_CanPersonExist` 在同刻/同区可见 true，在不同区可见 false。
4. 人物 `Dead` 后 `BS_CanPersonExist` 一律 false。
5. `DebugSweepExpired` 后过期区间自动移除。

如果你愿意，我们下一条开始 Day4：  
- `LocalPersonManagerSubsystem`（附近人实例化/回收）  
- 与 Day3 的 Commitment-Presence 形成可回收测试用例（避免“今天前线明天见面”）。
