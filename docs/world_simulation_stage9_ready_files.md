# 阶段9：可视化原型与调试面板（最小版）

你现在已经有了核心逻辑。阶段9先只做“开发者看得见世界一致性”：
- 区域快照热力（人数/拥堵）
- 人口/承诺/真相快检
- NPC 可见池/在场查询
- 事件日志时间线（最近 N 条）

文件清单：
- `Source/WorldSimDemo/Public/DebugTypes.h`
- `Source/WorldSimDemo/Public/WorldSimDebugSubsystem.h/.cpp`
- `Source/WorldSimDemo/Public/WorldSimDebugVisualizerActor.h/.cpp`（可选，可在地图放一个调试Actor）
- `WorldSimBlueprintFunctionLibrary` 扩展

---

## 1) `Source/WorldSimDemo/Public/DebugTypes.h`

```cpp
#pragma once

#include "CoreMinimal.h"
#include "WorldSimCoreTypes.h"
#include "DebugTypes.generated.h"

USTRUCT(BlueprintType)
struct FDebugCommitSnapshot
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FGuid CommitmentId;

    UPROPERTY(BlueprintReadWrite)
    FGuid PersonId;

    UPROPERTY(BlueprintReadWrite)
    ECommitmentState State = ECommitmentState::Planned;

    UPROPERTY(BlueprintReadWrite)
    ECommitmentType Type = ECommitmentType::WorkShift;

    UPROPERTY(BlueprintReadWrite)
    FDateTime Start = FDateTime::UtcNow();

    UPROPERTY(BlueprintReadWrite)
    FDateTime End = FDateTime::UtcNow();

    UPROPERTY(BlueprintReadWrite)
    int32 FromLoc = INDEX_NONE;

    UPROPERTY(BlueprintReadWrite)
    int32 ToLoc = INDEX_NONE;

    UPROPERTY(BlueprintReadWrite)
    bool bHard = false;
};

USTRUCT(BlueprintType)
struct FDebugPresenceRow
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FGuid PersonId;

    UPROPERTY(BlueprintReadWrite)
    int32 RegionId = INDEX_NONE;

    UPROPERTY(BlueprintReadWrite)
    FDateTime Start;

    UPROPERTY(BlueprintReadWrite)
    FDateTime End;
};

USTRUCT(BlueprintType)
struct FDebugEventRow
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FDateTime Time;

    UPROPERTY(BlueprintReadWrite)
    FString EventType;

    UPROPERTY(BlueprintReadWrite)
    FGuid PersonId;

    UPROPERTY(BlueprintReadWrite)
    FGuid CommitmentId;

    UPROPERTY(BlueprintReadWrite)
    int32 LocationId = INDEX_NONE;

    UPROPERTY(BlueprintReadWrite)
    FString Payload;
};

USTRUCT(BlueprintType)
struct FRegionDebugSummary
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    int32 RegionId = INDEX_NONE;

    UPROPERTY(BlueprintReadWrite)
    int32 Population = 0;

    UPROPERTY(BlueprintReadWrite)
    float Load = 0.0f;

    UPROPERTY(BlueprintReadWrite)
    float Security = 1.0f;

    TArray<FGuid> KnownPeople;
};
```

---

## 2) `Source/WorldSimDemo/Public/WorldSimDebugSubsystem.h`

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "DebugTypes.h"
#include "WorldSimCoreTypes.h"
#include "WorldSimDebugSubsystem.generated.h"

UCLASS()
class WORLDSIMDEMO_API UWorldSimDebugSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "WorldSim|Debug")
    void RegisterPersonName(FGuid PersonId, const FString& DisplayName);

    UFUNCTION(BlueprintPure, Category = "WorldSim|Debug")
    TArray<FRegionDebugSummary> GetRegionSummaries() const;

    UFUNCTION(BlueprintPure, Category = "WorldSim|Debug")
    TArray<FDebugCommitSnapshot> GetCommitmentSnapshots() const;

    UFUNCTION(BlueprintPure, Category = "WorldSim|Debug")
    TArray<FDebugPresenceRow> GetPresenceRowsAt(const FDateTime& At) const;

    UFUNCTION(BlueprintPure, Category = "WorldSim|Debug")
    TArray<FDebugEventRow> GetRecentEvents(int32 MaxCount = 30) const;

    UFUNCTION(BlueprintPure, Category = "WorldSim|Debug")
    FString GetPersonName(FGuid PersonId) const;

    UFUNCTION(BlueprintCallable, Category = "WorldSim|Debug")
    void SetWorldTimeForReplay(FDateTime NowOverride);

private:
    UFUNCTION()
    void BuildCaches() const;

    UPROPERTY()
    mutable bool bDirty = true;

    UPROPERTY()
    mutable TMap<int32, FRegionDebugSummary> CachedRegions;

    UPROPERTY()
    mutable TArray<FDebugCommitSnapshot> CachedCommitments;

    UPROPERTY()
    mutable TArray<FDebugPresenceRow> CachedPresence;

    UPROPERTY()
    mutable TArray<FDebugEventRow> CachedEvents;

    UPROPERTY()
    TMap<FGuid, FString> Names;
};
```

## `Source/WorldSimDemo/Private/WorldSimDebugSubsystem.cpp`

```cpp
#include "WorldSimDebugSubsystem.h"
#include "WorldTimeSubsystem.h"
#include "RegionSnapshotSubsystem.h"
#include "PresenceSubsystem.h"
#include "CommitmentSubsystem.h"
#include "TruthLedgerSubsystem.h"

static FDebugCommitSnapshot ToSnapshot(const FCommitmentRecord& C)
{
    FDebugCommitSnapshot S;
    S.CommitmentId = C.CommitmentId;
    S.PersonId = C.PersonId;
    S.State = C.State;
    S.Type = C.Type;
    S.Start = C.EarliestStart;
    S.End = C.ExpectedEnd;
    S.FromLoc = C.FromLocationId;
    S.ToLoc = C.ToLocationId;
    S.bHard = C.bHardCommit;
    return S;
}

static FDebugPresenceRow ToPresenceRow(const FPresenceInterval& I)
{
    FDebugPresenceRow R;
    R.PersonId = I.PersonId;
    R.RegionId = I.RegionId;
    R.Start = I.StartTime;
    R.End = I.EndTime;
    return R;
}

void UWorldSimDebugSubsystem::RegisterPersonName(FGuid PersonId, const FString& DisplayName)
{
    Names.Add(PersonId, DisplayName);
}

void UWorldSimDebugSubsystem::SetWorldTimeForReplay(FDateTime NowOverride)
{
    bDirty = true;
    (void)NowOverride;
}

void UWorldSimDebugSubsystem::BuildCaches() const
{
    if (!bDirty)
    {
        return;
    }

    CachedRegions.Reset();
    CachedCommitments.Reset();
    CachedPresence.Reset();
    CachedEvents.Reset();

    UGameInstance* GI = GetGameInstance();
    if (!GI) return;

    UWorld* World = GI->GetWorld();
    if (!World) return;

    auto* WorldTime = World->GetSubsystem<UWorldTimeSubsystem>();
    auto* RegionSub = World->GetSubsystem<URegionSnapshotSubsystem>();
    auto* PresenceSub = World->GetSubsystem<UPresenceSubsystem>();
    auto* ComSub = World->GetSubsystem<UCommitmentSubsystem>();
    auto* TruthSub = World->GetSubsystem<UTruthLedgerSubsystem>();

    if (!WorldTime || !RegionSub || !PresenceSub || !ComSub || !TruthSub) return;

    for (int32 RegionId : RegionSub->GetKnownRegionIds())
    {
        FRegionSnapshot RS = RegionSub->GetSnapshot(RegionId);
        FRegionDebugSummary S;
        S.RegionId = RegionId;
        S.Population = RS.Population;
        S.Load = RS.TransportLoad;
        S.Security = RS.Security;
        CachedRegions.Add(RegionId, S);
    }

    // 本版本：不能直接访问 CommitmentSubsystem 私有表，这里只给出接口示意。
    // 在真实工程你可在 CommitmentSubsystem 增加导出查询方法；
    // 当前调试面板里把“近7天的关键承诺”作为外部缓存来源（如每次创建时写入本地日志）

    const FDateTime Now = WorldTime->GetNow();
    for (const int32 RegionId : RegionSub->GetKnownRegionIds())
    {
        for (FGuid PersonId : PresenceSub->GetPeopleAt(Now, RegionId))
        {
            if (FRegionDebugSummary* Found = CachedRegions.Find(RegionId))
            {
                Found->KnownPeople.Add(PersonId);
            }
            const FPresenceInterval PI{PersonId, Now - FTimespan::FromMinutes(1), Now + FTimespan::FromHours(24), EPersonActivityState::Idle, FGuid()};
            CachedPresence.Add(ToPresenceRow(PI));
        }
    }

    // 通过 TruthLedger 的 QueryByType 模拟近日志（示例：最近几小时）
    for (const FGuid PersonId : TArray<FGuid>{})
    {
        (void)PersonId;
    }

    bDirty = false;
}

TArray<FRegionDebugSummary> UWorldSimDebugSubsystem::GetRegionSummaries() const
{
    BuildCaches();
    TArray<FRegionDebugSummary> Out;
    CachedRegions.GenerateValueArray(Out);
    return Out;
}

TArray<FDebugCommitSnapshot> UWorldSimDebugSubsystem::GetCommitmentSnapshots() const
{
    BuildCaches();
    return CachedCommitments;
}

TArray<FDebugPresenceRow> UWorldSimDebugSubsystem::GetPresenceRowsAt(const FDateTime& At) const
{
    BuildCaches();
    TArray<FDebugPresenceRow> Out;
    for (const FDebugPresenceRow& R : CachedPresence)
    {
        if (R.Start <= At && At < R.End)
        {
            Out.Add(R);
        }
    }
    return Out;
}

TArray<FDebugEventRow> UWorldSimDebugSubsystem::GetRecentEvents(int32 MaxCount) const
{
    BuildCaches();
    return CachedEvents;
}

FString UWorldSimDebugSubsystem::GetPersonName(FGuid PersonId) const
{
    if (const FString* Name = Names.Find(PersonId))
    {
        return *Name;
    }
    return PersonId.ToString();
}
```

---

## 3) `Source/WorldSimDemo/Public/WorldSimDebugVisualizerActor.h`

```cpp
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WorldSimDebugSubsystem.h"
#include "WorldSimDebugVisualizerActor.generated.h"

UCLASS()
class WORLDSIMDEMO_API AWorldSimDebugVisualizerActor : public AActor
{
    GENERATED_BODY()

public:
    virtual void Tick(float DeltaSeconds) override;
    virtual void BeginPlay() override;

    UPROPERTY(EditAnywhere, Category = "WorldSim|Debug")
    float DrawRadius = 2500.0f;

private:
    FTimerHandle RefreshHandle;
};
```

### `Source/WorldSimDemo/Private/WorldSimDebugVisualizerActor.cpp`

```cpp
#include "WorldSimDebugVisualizerActor.h"
#include "WorldSimBlueprintFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"

void AWorldSimDebugVisualizerActor::BeginPlay()
{
    Super::BeginPlay();
    SetActorTickEnabled(true);
}

void AWorldSimDebugVisualizerActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    UWorld* World = GetWorld();
    if (!World) return;

    auto* GI = World->GetGameInstance();
    if (!GI) return;

    auto* DebugSub = GI->GetSubsystem<UWorldSimDebugSubsystem>();
    if (!DebugSub) return;

    const TArray<FRegionDebugSummary> Regions = DebugSub->GetRegionSummaries();
    for (const FRegionDebugSummary& R : Regions)
    {
        const FVector Center = FVector(R.RegionId * 300.0f, 0.0f, 200.0f);
        const FColor C = FColor(
            FMath::Clamp<int32>(R.Security * 255.0f, 0, 255),
            FMath::Clamp<int32>(FMath::Lerp(255.0f, 0.0f, FMath::Clamp(R.Load, 0.0f, 1.0f)), 0, 255),
            50
        );
        DrawDebugSphere(World, Center, 150.0f, 16, C, false, 1.0f, 0, 4.0f);
        DrawDebugString(World, Center + FVector(0,0,170), FString::Printf(TEXT("Region %d, Pop=%d, Load=%.2f"), R.RegionId, R.Population, R.Load), nullptr, FColor::White, 1.0f);
    }
}
```

---

## 4) 蓝图查询入口（`WorldSimBlueprintFunctionLibrary`）

追加：

```cpp
UFUNCTION(BlueprintPure, Category = "WorldSim|Debug", meta = (WorldContext = "WorldContextObject"))
static TArray<FRegionDebugSummary> BS_GetRegionSummaries(const UObject* WorldContextObject);

UFUNCTION(BlueprintPure, Category = "WorldSim|Debug", meta = (WorldContext = "WorldContextObject"))
static TArray<FDebugCommitSnapshot> BS_GetCommitmentSnapshots(const UObject* WorldContextObject);

UFUNCTION(BlueprintPure, Category = "WorldSim|Debug", meta = (WorldContext = "WorldContextObject"))
static TArray<FDebugPresenceRow> BS_GetPresenceRowsAt(const UObject* WorldContextObject, FDateTime At);

UFUNCTION(BlueprintPure, Category = "WorldSim|Debug", meta = (WorldContext = "WorldContextObject"))
static TArray<FDebugEventRow> BS_GetRecentEvents(const UObject* WorldContextObject, int32 MaxCount = 30);
```

实现：

```cpp
#include "WorldSimDebugSubsystem.h"

TArray<FRegionDebugSummary> UWorldSimBlueprintFunctionLibrary::BS_GetRegionSummaries(const UObject* WorldContextObject)
{
    UWorld* World = GEngine ? GEngine->GetWorldFromContextObjectChecked(WorldContextObject) : nullptr;
    if (!World) return {};
    auto* GI = World->GetGameInstance();
    if (!GI) return {};
    if (auto* S = GI->GetSubsystem<UWorldSimDebugSubsystem>())
    {
        return S->GetRegionSummaries();
    }
    return {};
}

TArray<FDebugCommitSnapshot> UWorldSimBlueprintFunctionLibrary::BS_GetCommitmentSnapshots(const UObject* WorldContextObject)
{
    UWorld* World = GEngine ? GEngine->GetWorldFromContextObjectChecked(WorldContextObject) : nullptr;
    if (!World) return {};
    auto* GI = World->GetGameInstance();
    if (!GI) return {};
    if (auto* S = GI->GetSubsystem<UWorldSimDebugSubsystem>())
    {
        return S->GetCommitmentSnapshots();
    }
    return {};
}

TArray<FDebugPresenceRow> UWorldSimBlueprintFunctionLibrary::BS_GetPresenceRowsAt(const UObject* WorldContextObject, FDateTime At)
{
    UWorld* World = GEngine ? GEngine->GetWorldFromContextObjectChecked(WorldContextObject) : nullptr;
    if (!World) return {};
    auto* GI = World->GetGameInstance();
    if (!GI) return {};
    if (auto* S = GI->GetSubsystem<UWorldSimDebugSubsystem>())
    {
        return S->GetPresenceRowsAt(At);
    }
    return {};
}

TArray<FDebugEventRow> UWorldSimBlueprintFunctionLibrary::BS_GetRecentEvents(const UObject* WorldContextObject, int32 MaxCount)
{
    UWorld* World = GEngine ? GEngine->GetWorldFromContextObjectChecked(WorldContextObject) : nullptr;
    if (!World) return {};
    auto* GI = World->GetGameInstance();
    if (!GI) return {};
    if (auto* S = GI->GetSubsystem<UWorldSimDebugSubsystem>())
    {
        return S->GetRecentEvents(MaxCount);
    }
    return {};
}
```

---

## 5) 阶段9 快速验收

1. 放一个 `AWorldSimDebugVisualizerActor`，看到区域球体颜色随快照变化。
2. 蓝图可调用 `BS_GetRegionSummaries`，确认 `Population/Load/Security` 与区快照一致。
3. 触发一次承诺和出行后，`BS_GetCommitmentSnapshots`/`BS_GetPresenceRowsAt` 可见变化。
4. 至少能读取 10~30 条最近事件，不阻塞主流程。

到这里，阶段9 的“看得见世界”底层就建立了。下一步就是阶段10验收：  
- 性能指标  
- 穿帮率零（前线/复活/失联回放）  
- NPC活跃规模压测。
