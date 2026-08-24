# Day2 可直接贴入 UE5 的代码（区域快照 + 真相日志）

这一步建立你世界真相层的最小形态。  
前置：已贴好 Day1 的文件（见 `world_simulation_day1_ready_files.md`）。

---

## 1. `Source/WorldSimDemo/Public/RegionSnapshotSubsystem.h`

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WorldSimCoreTypes.h"
#include "RegionSnapshotSubsystem.generated.h"

UCLASS()
class WORLDSIMDEMO_API URegionSnapshotSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "WorldSim|Region")
    void EnsureRegion(int32 RegionId);

    UFUNCTION(BlueprintCallable, Category = "WorldSim|Region")
    FRegionSnapshot GetSnapshot(int32 RegionId) const;

    UFUNCTION(BlueprintCallable, Category = "WorldSim|Region")
    void SetSnapshot(const FRegionSnapshot& Snapshot);

    UFUNCTION(BlueprintCallable, Category = "WorldSim|Region")
    void ApplyDelta(int32 RegionId, int32 PopulationDelta, float SecurityDelta = 0.0f, float EconomyDelta = 0.0f);

    UFUNCTION(BlueprintCallable, Category = "WorldSim|Region")
    void ApplyTransportDelta(int32 RegionId, float TransportDelta);

    UFUNCTION(BlueprintCallable, Category = "WorldSim|Region")
    void TickRegion(float DeltaSeconds);

    UFUNCTION(BlueprintPure, Category = "WorldSim|Region")
    TArray<int32> GetKnownRegionIds() const;

private:
    UFUNCTION()
    FRegionSnapshot& GetOrCreate(int32 RegionId);

    UPROPERTY()
    TMap<int32, FRegionSnapshot> Snapshots;
};
```

## `Source/WorldSimDemo/Private/RegionSnapshotSubsystem.cpp`

```cpp
#include "RegionSnapshotSubsystem.h"

FRegionSnapshot& URegionSnapshotSubsystem::GetOrCreate(int32 RegionId)
{
    FRegionSnapshot& Snapshot = Snapshots.FindOrAdd(RegionId);
    if (Snapshot.RegionId != RegionId)
    {
        Snapshot.RegionId = RegionId;
        if (Snapshot.WindowStart.GetTicks() == 0 && Snapshot.WindowEnd.GetTicks() == 0)
        {
            const FDateTime Now = FDateTime::UtcNow();
            Snapshot.WindowStart = Now;
            Snapshot.WindowEnd = Now;
        }
    }
    return Snapshot;
}

void URegionSnapshotSubsystem::EnsureRegion(int32 RegionId)
{
    GetOrCreate(RegionId);
}

FRegionSnapshot URegionSnapshotSubsystem::GetSnapshot(int32 RegionId) const
{
    if (const FRegionSnapshot* Found = Snapshots.Find(RegionId))
    {
        return *Found;
    }

    FRegionSnapshot Empty;
    Empty.RegionId = RegionId;
    return Empty;
}

void URegionSnapshotSubsystem::SetSnapshot(const FRegionSnapshot& Snapshot)
{
    FRegionSnapshot& Target = GetOrCreate(Snapshot.RegionId);
    Target = Snapshot;
    if (Target.WindowStart > Target.WindowEnd)
    {
        Target.WindowEnd = Target.WindowStart;
    }
}

void URegionSnapshotSubsystem::ApplyDelta(int32 RegionId, int32 PopulationDelta, float SecurityDelta, float EconomyDelta)
{
    FRegionSnapshot& S = GetOrCreate(RegionId);
    S.Population = FMath::Max(0, S.Population + PopulationDelta);
    S.Security = FMath::Max(0.0f, S.Security + SecurityDelta);
    S.EconomyPressure = FMath::Max(0.0f, S.EconomyPressure + EconomyDelta);
}

void URegionSnapshotSubsystem::ApplyTransportDelta(int32 RegionId, float TransportDelta)
{
    FRegionSnapshot& S = GetOrCreate(RegionId);
    S.TransportLoad = FMath::Max(0.0f, S.TransportLoad + TransportDelta);
}

void URegionSnapshotSubsystem::TickRegion(float DeltaSeconds)
{
    const FTimespan Delta = FTimespan::FromSeconds(DeltaSeconds);
    for (auto& Pair : Snapshots)
    {
        FRegionSnapshot& S = Pair.Value;
        S.WindowEnd += Delta;

        // 简单衰减：压力型指标慢慢回落，让数值不持续发散
        S.TransportLoad = FMath::Max(0.0f, S.TransportLoad - 0.001f * DeltaSeconds);
        S.EconomyPressure = FMath::Max(0.0f, S.EconomyPressure - 0.001f * DeltaSeconds);
    }
}

TArray<int32> URegionSnapshotSubsystem::GetKnownRegionIds() const
{
    TArray<int32> Out;
    Snapshots.GetKeys(Out);
    return Out;
}
```

> 注意：`FRegionSnapshot` 里我们用了 `WindowStart/WindowEnd`，日后可扩展为离散时间片（小时/天）统计桶。

---

## 2. `Source/WorldSimDemo/Public/TruthLedgerSubsystem.h`

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WorldSimCoreTypes.h"
#include "TruthLedgerSubsystem.generated.h"

UCLASS()
class WORLDSIMDEMO_API UTruthLedgerSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "WorldSim|Truth")
    void RecordEvent(const FWorldEvent& Event);

    UFUNCTION(BlueprintCallable, Category = "WorldSim|Truth")
    bool ResolveLatestState(FGuid PersonId, EExistenceState& OutState, FWorldEvent& OutEvidence) const;

    UFUNCTION(BlueprintCallable, Category = "WorldSim|Truth")
    TArray<FWorldEvent> QueryEventsByPerson(FGuid PersonId, FDateTime From, FDateTime To) const;

    UFUNCTION(BlueprintCallable, Category = "WorldSim|Truth")
    TArray<FWorldEvent> QueryByType(FName EventType, FDateTime From, FDateTime To) const;

    UFUNCTION(BlueprintCallable, Category = "WorldSim|Truth")
    bool HasHardConflict(FGuid PersonId, int32 RegionId, FDateTime At) const;

private:
    UPROPERTY()
    TArray<FWorldEvent> Events;
};
```

## `Source/WorldSimDemo/Private/TruthLedgerSubsystem.cpp`

```cpp
#include "TruthLedgerSubsystem.h"

void UTruthLedgerSubsystem::RecordEvent(const FWorldEvent& Event)
{
    if (!Event.EventId.IsValid())
    {
        FWorldEvent Copy = Event;
        Copy.EventId = FGuid::NewGuid();
        Events.Add(Copy);
        return;
    }

    Events.Add(Event);
}

bool UTruthLedgerSubsystem::ResolveLatestState(FGuid PersonId, EExistenceState& OutState, FWorldEvent& OutEvidence) const
{
    FDateTime Latest = FDateTime::MinValue();
    bool bFound = false;

    for (const FWorldEvent& E : Events)
    {
        if (E.PersonId != PersonId) continue;
        if (E.EventTime < Latest) continue;

        Latest = E.EventTime;
        OutEvidence = E;
        bFound = true;
    }

    if (!bFound)
    {
        OutState = EExistenceState::Unknown;
        return false;
    }

    if (OutEvidence.EventType == TEXT("Dead"))
    {
        OutState = EExistenceState::Dead;
    }
    else if (OutEvidence.EventType == TEXT("Wounded"))
    {
        OutState = EExistenceState::Wounded;
    }
    else if (OutEvidence.EventType == TEXT("Alive") || OutEvidence.EventType == TEXT("Returned") || OutEvidence.EventType == TEXT("Arrived"))
    {
        OutState = EExistenceState::Alive;
    }
    else
    {
        OutState = EExistenceState::Unknown;
    }
    return true;
}

TArray<FWorldEvent> UTruthLedgerSubsystem::QueryEventsByPerson(FGuid PersonId, FDateTime From, FDateTime To) const
{
    TArray<FWorldEvent> Out;
    for (const FWorldEvent& E : Events)
    {
        if (E.PersonId == PersonId && E.EventTime >= From && E.EventTime <= To)
        {
            Out.Add(E);
        }
    }
    return Out;
}

TArray<FWorldEvent> UTruthLedgerSubsystem::QueryByType(FName EventType, FDateTime From, FDateTime To) const
{
    TArray<FWorldEvent> Out;
    for (const FWorldEvent& E : Events)
    {
        if (E.EventType == EventType && E.EventTime >= From && E.EventTime <= To)
        {
            Out.Add(E);
        }
    }
    return Out;
}

bool UTruthLedgerSubsystem::HasHardConflict(FGuid PersonId, int32 RegionId, FDateTime At) const
{
    // Day2 版本规则：同人物同时间若出现“高可信在场承诺类事件”，认为冲突
    for (const FWorldEvent& E : Events)
    {
        if (E.PersonId != PersonId || E.LocationId != RegionId)
        {
            continue;
        }

        if (E.EventTime <= At &&
            (E.EventType == TEXT("Arrived") || E.EventType == TEXT("Deployed") || E.EventType == TEXT("Deported")))
        {
            return true;
        }
    }
    return false;
}
```

---

## 3. 扩展 `WorldSimBlueprintFunctionLibrary.h`

新增两个接口（接入 Day1 文件）：

```cpp
// WorldSimBlueprintFunctionLibrary.h
UFUNCTION(BlueprintPure, Category = "WorldSim|API", meta = (WorldContext = "WorldContextObject"))
static FRegionSnapshot BS_GetRegionSnapshot(const UObject* WorldContextObject, int32 RegionId);

UFUNCTION(BlueprintPure, Category = "WorldSim|API", meta = (WorldContext = "WorldContextObject"))
static EExistenceState BS_QueryTruth(const UObject* WorldContextObject, FGuid PersonId);
```

## `WorldSimBlueprintFunctionLibrary.cpp` 追加实现

```cpp
#include "RegionSnapshotSubsystem.h"
#include "TruthLedgerSubsystem.h"

FRegionSnapshot UWorldSimBlueprintFunctionLibrary::BS_GetRegionSnapshot(const UObject* WorldContextObject, int32 RegionId)
{
    if (!WorldContextObject)
    {
        return FRegionSnapshot();
    }

    UWorld* World = GEngine ? GEngine->GetWorldFromContextObjectChecked(WorldContextObject) : nullptr;
    if (!World)
    {
        return FRegionSnapshot();
    }

    if (auto* RegionSub = World->GetSubsystem<URegionSnapshotSubsystem>())
    {
        return RegionSub->GetSnapshot(RegionId);
    }

    return FRegionSnapshot();
}

EExistenceState UWorldSimBlueprintFunctionLibrary::BS_QueryTruth(const UObject* WorldContextObject, FGuid PersonId)
{
    if (!WorldContextObject)
    {
        return EExistenceState::Unknown;
    }

    UWorld* World = GEngine ? GEngine->GetWorldFromContextObjectChecked(WorldContextObject) : nullptr;
    if (!World)
    {
        return EExistenceState::Unknown;
    }

    if (auto* TruthSub = World->GetSubsystem<UTruthLedgerSubsystem>())
    {
        EExistenceState State;
        FWorldEvent Dummy;
        if (TruthSub->ResolveLatestState(PersonId, State, Dummy))
        {
            return State;
        }
    }

    return EExistenceState::Unknown;
}
```

---

## 4. Day2 开发验收（强制）

1. 通过编辑器编译。
2. 蓝图调用：
   - `BS_GetRegionSnapshot` 能返回有效 `FRegionSnapshot`。
   - `BS_QueryTruth` 在未有事件时返回 `Unknown`。
3. 用 `RecordEvent` 写入一条 `EventType="Dead"`，同人查询变成 `Dead`。
4. `ApplyDelta` / `ApplyTransportDelta` 改变区域快照数值且有上限下限控制。

完成这 4 点后，阶段1 Day2 可以算通过，下一步直接 Day3 承诺 + 存在性闭环。
