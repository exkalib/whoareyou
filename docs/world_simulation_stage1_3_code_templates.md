# 阶段1~3 代码骨架模板（可直接贴入UE5项目）

> 当前仓库检测不到 `.uproject`。请先在UE里建好项目（建议 `WorldSimDemo`），然后将以下文件放入 `Source/WorldSimDemo/`。

---

## 1) `WorldSimTypes.h`
```cpp
#pragma once

#include "CoreMinimal.h"
#include "WorldSimTypes.generated.h"

UENUM(BlueprintType)
enum class EExistenceState : uint8
{
    Alive UMETA(DisplayName = "Alive"),
    Wounded UMETA(DisplayName = "Wounded"),
    Dead UMETA(DisplayName = "Dead"),
    Unknown UMETA(DisplayName = "Unknown")
};

UENUM(BlueprintType)
enum class EPersonActivityState : uint8
{
    Idle UMETA(DisplayName = "Idle"),
    Committed UMETA(DisplayName = "Committed"),
    Departing UMETA(DisplayName = "Departing"),
    InTransit UMETA(DisplayName = "InTransit"),
    InOperation UMETA(DisplayName = "InOperation"),
    Returning UMETA(DisplayName = "Returning"),
    Recovered UMETA(DisplayName = "Recovered"),
    Offline UMETA(DisplayName = "Offline")
};

UENUM(BlueprintType)
enum class ECommitmentType : uint8
{
    WorkShift UMETA(DisplayName = "WorkShift"),
    Transit UMETA(DisplayName = "Transit"),
    Military UMETA(DisplayName = "Military"),
    BusinessTrip UMETA(DisplayName = "BusinessTrip"),
    Tourism UMETA(DisplayName = "Tourism"),
    Rescue UMETA(DisplayName = "Rescue"),
    Migration UMETA(DisplayName = "Migration")
};

UENUM(BlueprintType)
enum class EKnowledgeConfidence : uint8
{
    Low UMETA(DisplayName = "Low"),
    Medium UMETA(DisplayName = "Medium"),
    High UMETA(DisplayName = "High"),
    Verified UMETA(DisplayName = "Verified")
};

USTRUCT(BlueprintType)
struct FRegionSnapshot
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    int32 RegionId = INDEX_NONE;

    UPROPERTY(BlueprintReadWrite)
    FDateTime WindowStart = FDateTime::Now();

    UPROPERTY(BlueprintReadWrite)
    FDateTime WindowEnd = FDateTime::Now();

    UPROPERTY(BlueprintReadWrite)
    int32 Population = 0;

    UPROPERTY(BlueprintReadWrite)
    float FoodPrice = 1.0f;

    UPROPERTY(BlueprintReadWrite)
    float TransportLoad = 0.0f;

    UPROPERTY(BlueprintReadWrite)
    float Security = 1.0f;

    UPROPERTY(BlueprintReadWrite)
    float EconomyPressure = 0.0f;

    UPROPERTY(BlueprintReadWrite)
    int32 IncomingFlow = 0;

    UPROPERTY(BlueprintReadWrite)
    int32 OutgoingFlow = 0;

    UPROPERTY(BlueprintReadWrite)
    int32 ActiveCommitments = 0;
};

USTRUCT(BlueprintType)
struct FPersonLiteState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FGuid PersonId;

    UPROPERTY(BlueprintReadWrite)
    int32 HomeRegionId = INDEX_NONE;

    UPROPERTY(BlueprintReadWrite)
    EExistenceState LifeState = EExistenceState::Unknown;

    UPROPERTY(BlueprintReadWrite)
    FDateTime LockedUntil = FDateTime::MinValue();

    UPROPERTY(BlueprintReadWrite)
    int32 WealthBand = 0;

    UPROPERTY(BlueprintReadWrite)
    FString OccupationCode;

    UPROPERTY(BlueprintReadWrite)
    TArray<FGuid> ActiveCommitmentIds;
};

USTRUCT(BlueprintType)
struct FPersonFullState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FGuid PersonId;

    UPROPERTY(BlueprintReadWrite)
    FTransform WorldTransform = FTransform::Identity;

    UPROPERTY(BlueprintReadWrite)
    EPersonActivityState ActivityState = EPersonActivityState::Idle;

    UPROPERTY(BlueprintReadWrite)
    FDateTime NextPlanTime = FDateTime::Now();

    UPROPERTY(BlueprintReadWrite)
    TArray<FString> ActiveTags;

    UPROPERTY(BlueprintReadWrite)
    FVector2D EmotionalVec = FVector2D::ZeroVector;

    UPROPERTY(BlueprintReadWrite)
    float Health = 1.0f;

    UPROPERTY(BlueprintReadWrite)
    int32 RegionCellId = INDEX_NONE;
};

USTRUCT(BlueprintType)
struct FCommitmentEvent
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FGuid CommitmentId;

    UPROPERTY(BlueprintReadWrite)
    FGuid PersonId;

    UPROPERTY(BlueprintReadWrite)
    ECommitmentType Type = ECommitmentType::WorkShift;

    UPROPERTY(BlueprintReadWrite)
    FDateTime EarliestStart = FDateTime::Now();

    UPROPERTY(BlueprintReadWrite)
    FDateTime LatestStart = FDateTime::Now();

    UPROPERTY(BlueprintReadWrite)
    FDateTime ExpectedEnd = FDateTime::Now();

    UPROPERTY(BlueprintReadWrite)
    int32 FromLocationId = INDEX_NONE;

    UPROPERTY(BlueprintReadWrite)
    int32 ToLocationId = INDEX_NONE;

    UPROPERTY(BlueprintReadWrite)
    int32 RouteId = INDEX_NONE;

    UPROPERTY(BlueprintReadWrite)
    bool bHardCommit = false;

    UPROPERTY(BlueprintReadWrite)
    float CostBudget = 0.0f;

    UPROPERTY(BlueprintReadWrite)
    bool bCancelable = true;

    UPROPERTY(BlueprintReadWrite)
    bool bExecuted = false;
};

USTRUCT(BlueprintType)
struct FPresenceInterval
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FGuid PersonId;

    UPROPERTY(BlueprintReadWrite)
    FDateTime StartTime = FDateTime::MinValue();

    UPROPERTY(BlueprintReadWrite)
    FDateTime EndTime = FDateTime::MaxValue();

    UPROPERTY(BlueprintReadWrite)
    int32 LocationId = INDEX_NONE;

    UPROPERTY(BlueprintReadWrite)
    EPersonActivityState StateTag = EPersonActivityState::Idle;

    UPROPERTY(BlueprintReadWrite)
    FGuid CommitmentId;
};

USTRUCT(BlueprintType)
struct FWorldEvent
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FGuid EventId;

    UPROPERTY(BlueprintReadWrite)
    FGuid SourceCommitmentId;

    UPROPERTY(BlueprintReadWrite)
    FGuid PersonId;

    UPROPERTY(BlueprintReadWrite)
    FString EventType;

    UPROPERTY(BlueprintReadWrite)
    FDateTime EventTime = FDateTime::Now();

    UPROPERTY(BlueprintReadWrite)
    int32 LocationId = INDEX_NONE;

    UPROPERTY(BlueprintReadWrite)
    FString JsonPayload;
};

USTRUCT(BlueprintType)
struct FPlayerKnowledgeItem
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FGuid FactId;

    UPROPERTY(BlueprintReadWrite)
    FString Source;

    UPROPERTY(BlueprintReadWrite)
    EKnowledgeConfidence Confidence = EKnowledgeConfidence::Low;

    UPROPERTY(BlueprintReadWrite)
    FString Content;

    UPROPERTY(BlueprintReadWrite)
    FDateTime ObservedAt = FDateTime::Now();

    UPROPERTY(BlueprintReadWrite)
    bool bVerified = false;
};

USTRUCT(BlueprintType)
struct FPlayerCharacterInput
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FString Gender;

    UPROPERTY(BlueprintReadWrite)
    FString AppearanceSeed;

    UPROPERTY(BlueprintReadWrite)
    int32 CulturalPreference = 0;

    UPROPERTY(BlueprintReadWrite)
    int32 CareerInterest = 0;
};
```

---

## 2) `WorldTimeSubsystem.h`
```cpp
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WorldSimTypes.h"
#include "WorldTimeSubsystem.generated.h"

UCLASS()
class WORLDSIMDEMO_API UWorldTimeSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintPure, Category="WorldSim|Time")
    FDateTime GetNow() const;

    UFUNCTION(BlueprintCallable, Category="WorldSim|Time")
    void AdvanceWorldTime(float DeltaSeconds);

    UFUNCTION(BlueprintCallable, Category="WorldSim|Time")
    void SetNow(const FDateTime& NewNow);

    UFUNCTION(BlueprintPure, Category="WorldSim|Time")
    FDateTime GetDayStart(const FDateTime& InTime) const;

    UFUNCTION(BlueprintPure, Category="WorldSim|Time")
    int32 DaysSinceStart() const;

private:
    UPROPERTY()
    FDateTime StartTime;

    UPROPERTY()
    FDateTime CurrentTime;
};
```

### `WorldTimeSubsystem.cpp`
```cpp
#include "WorldTimeSubsystem.h"

void UWorldTimeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    StartTime = FDateTime::UtcNow();
    CurrentTime = StartTime;
}

void UWorldTimeSubsystem::Deinitialize()
{
    Super::Deinitialize();
}

FDateTime UWorldTimeSubsystem::GetNow() const
{
    return CurrentTime;
}

void UWorldTimeSubsystem::AdvanceWorldTime(float DeltaSeconds)
{
    const FTimespan Added = FTimespan::FromSeconds(DeltaSeconds);
    CurrentTime += Added;
}

void UWorldTimeSubsystem::SetNow(const FDateTime& NewNow)
{
    CurrentTime = NewNow;
}

FDateTime UWorldTimeSubsystem::GetDayStart(const FDateTime& InTime) const
{
    return FDateTime(InTime.GetYear(), InTime.GetMonth(), InTime.GetDay(), 0, 0, 0);
}

int32 UWorldTimeSubsystem::DaysSinceStart() const
{
    return (CurrentTime - StartTime).GetDays();
}
```

---

## 3) `RegionSnapshotSubsystem.h/.cpp`
```cpp
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WorldSimTypes.h"
#include "RegionSnapshotSubsystem.generated.h"

UCLASS()
class WORLDSIMDEMO_API URegionSnapshotSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="WorldSim|Region")
    void EnsureRegion(int32 RegionId);

    UFUNCTION(BlueprintCallable, Category="WorldSim|Region")
    FRegionSnapshot GetSnapshot(int32 RegionId) const;

    UFUNCTION(BlueprintCallable, Category="WorldSim|Region")
    void AdvanceRegion(int32 RegionId, float DeltaHours);

    UFUNCTION(BlueprintCallable, Category="WorldSim|Region")
    void ApplyMicroDelta(int32 RegionId, int32 DeltaPopulation, int32 DeltaTraffic);

private:
    UPROPERTY()
    TMap<int32, FRegionSnapshot> Snapshots;
};
```

```cpp
#include "RegionSnapshotSubsystem.h"

void URegionSnapshotSubsystem::EnsureRegion(int32 RegionId)
{
    if (!Snapshots.Contains(RegionId))
    {
        FRegionSnapshot NewSnap;
        NewSnap.RegionId = RegionId;
        NewSnap.WindowStart = FDateTime::UtcNow();
        NewSnap.WindowEnd = NewSnap.WindowStart;
        Snapshots.Add(RegionId, NewSnap);
    }
}

FRegionSnapshot URegionSnapshotSubsystem::GetSnapshot(int32 RegionId) const
{
    if (const FRegionSnapshot* Found = Snapshots.Find(RegionId))
    {
        return *Found;
    }
    return FRegionSnapshot();
}

void URegionSnapshotSubsystem::AdvanceRegion(int32 RegionId, float DeltaHours)
{
    EnsureRegion(RegionId);
    // TODO: 热数据更新（供需变化、运力压力等）
    FRegionSnapshot& S = Snapshots[RegionId];
    S.WindowStart += FTimespan::FromHours(DeltaHours);
    S.WindowEnd = S.WindowStart;
}

void URegionSnapshotSubsystem::ApplyMicroDelta(int32 RegionId, int32 DeltaPopulation, int32 DeltaTraffic)
{
    EnsureRegion(RegionId);
    FRegionSnapshot& S = Snapshots[RegionId];
    S.Population = FMath::Max(0, S.Population + DeltaPopulation);
    S.TransportLoad = FMath::Max(0.0f, S.TransportLoad + DeltaTraffic * 0.01f);
}
```

---

## 4) `CommitmentSubsystem.h/.cpp`
```cpp
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WorldSimTypes.h"
#include "CommitmentSubsystem.generated.h"

UCLASS()
class WORLDSIMDEMO_API UCommitmentSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="WorldSim|Commitment")
    FGuid CreateCommitment(const FCommitmentEvent& InEvent);

    UFUNCTION(BlueprintCallable, Category="WorldSim|Commitment")
    bool CancelCommitment(FGuid CommitmentId);

    UFUNCTION(BlueprintCallable, Category="WorldSim|Commitment")
    bool UpdateCommitmentState(FGuid CommitmentId, const FString& NewState, const FString& Reason);

    UFUNCTION(BlueprintCallable, Category="WorldSim|Commitment")
    TArray<FGuid> GetCommitmentsForPerson(FGuid PersonId) const;

    UFUNCTION(BlueprintCallable, Category="WorldSim|Commitment")
    TArray<FGuid> GetDueCommitments(const FDateTime& Now, float LookAheadHours = 1.0f) const;

private:
    UPROPERTY()
    TMap<FGuid, FCommitmentEvent> Commitments;

    UPROPERTY()
    TMap<FGuid, TArray<FGuid>> PersonCommitments;
};
```

```cpp
#include "CommitmentSubsystem.h"

FGuid UCommitmentSubsystem::CreateCommitment(const FCommitmentEvent& InEvent)
{
    FCommitmentEvent Copy = InEvent;
    if (!Copy.CommitmentId.IsValid())
    {
        Copy.CommitmentId = FGuid::NewGuid();
    }

    Commitments.Add(Copy.CommitmentId, Copy);
    PersonCommitments.FindOrAdd(Copy.PersonId).Add(Copy.CommitmentId);
    return Copy.CommitmentId;
}

bool UCommitmentSubsystem::CancelCommitment(FGuid CommitmentId)
{
    if (!Commitments.Contains(CommitmentId))
    {
        return false;
    }

    const FGuid Person = Commitments[CommitmentId].PersonId;
    Commitments[CommitmentId].bCancelable = true;
    Commitments.Remove(CommitmentId);
    if (TArray<FGuid>* List = PersonCommitments.Find(Person))
    {
        List->Remove(CommitmentId);
    }
    return true;
}

bool UCommitmentSubsystem::UpdateCommitmentState(FGuid CommitmentId, const FString& NewState, const FString& Reason)
{
    if (!Commitments.Contains(CommitmentId))
    {
        return false;
    }

    // TODO: 把 NewState 存进事件元信息
    return true;
}

TArray<FGuid> UCommitmentSubsystem::GetCommitmentsForPerson(FGuid PersonId) const
{
    if (const TArray<FGuid>* List = PersonCommitments.Find(PersonId))
    {
        return *List;
    }
    return {};
}

TArray<FGuid> UCommitmentSubsystem::GetDueCommitments(const FDateTime& Now, float LookAheadHours) const
{
    TArray<FGuid> Due;
    for (const auto& Pair : Commitments)
    {
        const FCommitmentEvent& C = Pair.Value;
        if (!C.bExecuted && !C.bCancelable)
        {
            continue;
        }

        if (C.EarliestStart <= Now && C.LatestStart + FTimespan::FromHours(LookAheadHours) >= Now)
        {
            Due.Add(Pair.Key);
        }
    }
    return Due;
}
```

---

## 5) `PresenceSubsystem.h/.cpp`
```cpp
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WorldSimTypes.h"
#include "PresenceSubsystem.generated.h"

UCLASS()
class WORLDSIMDEMO_API UPresenceSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="WorldSim|Presence")
    bool LockPresence(const FPresenceInterval& Interval);

    UFUNCTION(BlueprintCallable, Category="WorldSim|Presence")
    bool CanAppear(FGuid PersonId, int32 RegionId, const FDateTime& At) const;

    UFUNCTION(BlueprintCallable, Category="WorldSim|Presence")
    bool IsActiveHere(FGuid PersonId, int32 RegionId, const FDateTime& At) const;

    UFUNCTION(BlueprintCallable, Category="WorldSim|Presence")
    TOptional<FPresenceInterval> GetInterval(FGuid PersonId) const;

    UFUNCTION(BlueprintCallable, Category="WorldSim|Presence")
    void ReleasePresence(FGuid PersonId, const FDateTime& At);

    UFUNCTION(BlueprintPure, Category="WorldSim|Presence")
    TArray<FGuid> GetPeopleInRegion(int32 RegionId) const;

private:
    UPROPERTY()
    TMap<FGuid, FPresenceInterval> ActiveIntervals;
};
```

```cpp
#include "PresenceSubsystem.h"

bool UPresenceSubsystem::LockPresence(const FPresenceInterval& Interval)
{
    const FGuid Person = Interval.PersonId;
    if (ActiveIntervals.Contains(Person))
    {
        const FPresenceInterval& Exist = ActiveIntervals[Person];
        if (Interval.StartTime < Exist.EndTime && Exist.StartTime < Interval.EndTime)
        {
            return false; // 时间重叠时，先拒绝（保持单点存在）
        }
    }

    ActiveIntervals.Add(Person, Interval);
    return true;
}

bool UPresenceSubsystem::CanAppear(FGuid PersonId, int32 RegionId, const FDateTime& At) const
{
    const TOptional<FPresenceInterval> Interval = GetInterval(PersonId);
    if (!Interval.IsSet())
    {
        return true;
    }

    const FPresenceInterval& I = Interval.GetValue();
    return I.StartTime <= At && At < I.EndTime && I.LocationId == RegionId;
}

bool UPresenceSubsystem::IsActiveHere(FGuid PersonId, int32 RegionId, const FDateTime& At) const
{
    return CanAppear(PersonId, RegionId, At);
}

TOptional<FPresenceInterval> UPresenceSubsystem::GetInterval(FGuid PersonId) const
{
    if (const FPresenceInterval* Interval = ActiveIntervals.Find(PersonId))
    {
        return TOptional<FPresenceInterval>(*Interval);
    }
    return {};
}

void UPresenceSubsystem::ReleasePresence(FGuid PersonId, const FDateTime& At)
{
    if (FPresenceInterval* Exist = ActiveIntervals.Find(PersonId))
    {
        if (Exist->EndTime > At)
        {
            Exist->EndTime = At;
        }
    }
}

TArray<FGuid> UPresenceSubsystem::GetPeopleInRegion(int32 RegionId) const
{
    TArray<FGuid> Out;
    for (const auto& Pair : ActiveIntervals)
    {
        if (Pair.Value.LocationId == RegionId)
        {
            Out.Add(Pair.Key);
        }
    }
    return Out;
}
```

---

## 6) `TruthLedgerSubsystem.h/.cpp`
```cpp
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WorldSimTypes.h"
#include "TruthLedgerSubsystem.generated.h"

UCLASS()
class WORLDSIMDEMO_API UTruthLedgerSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="WorldSim|Truth")
    void AppendEvent(const FWorldEvent& Event);

    UFUNCTION(BlueprintCallable, Category="WorldSim|Truth")
    bool TryGetLatestState(FGuid PersonId, EExistenceState& OutState, FWorldEvent& OutEvidence) const;

    UFUNCTION(BlueprintCallable, Category="WorldSim|Truth")
    TArray<FWorldEvent> QueryEvents(FGuid PersonId, FDateTime From, FDateTime To) const;

    UFUNCTION(BlueprintCallable, Category="WorldSim|Truth")
    bool HasHardCommitmentConflict(FGuid PersonId, int32 RegionId, const FDateTime& At) const;

private:
    UPROPERTY()
    TArray<FWorldEvent> Events;
};
```

```cpp
#include "TruthLedgerSubsystem.h"

void UTruthLedgerSubsystem::AppendEvent(const FWorldEvent& Event)
{
    Events.Add(Event);
}

bool UTruthLedgerSubsystem::TryGetLatestState(FGuid PersonId, EExistenceState& OutState, FWorldEvent& OutEvidence) const
{
    FDateTime Latest = FDateTime::MinValue();
    bool bFound = false;
    for (const FWorldEvent& E : Events)
    {
        if (E.PersonId != PersonId) continue;
        if (E.EventTime > Latest)
        {
            Latest = E.EventTime;
            OutEvidence = E;
            bFound = true;
        }
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
    else if (OutEvidence.EventType == TEXT("Returned") || OutEvidence.EventType == TEXT("Survived"))
    {
        OutState = EExistenceState::Alive;
    }
    else
    {
        OutState = EExistenceState::Alive;
    }

    return true;
}

TArray<FWorldEvent> UTruthLedgerSubsystem::QueryEvents(FGuid PersonId, FDateTime From, FDateTime To) const
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

bool UTruthLedgerSubsystem::HasHardCommitmentConflict(FGuid PersonId, int32 RegionId, const FDateTime& At) const
{
    for (const FWorldEvent& E : Events)
    {
        if (E.PersonId == PersonId && E.LocationId == RegionId && E.EventTime <= At)
        {
            return true;
        }
    }
    return false;
}
```

---

## 7) `LocalPersonManager.h/.cpp`
```cpp
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "WorldSimTypes.h"
#include "LocalPersonManager.generated.h"

UCLASS()
class WORLDSIMDEMO_API ULocalPersonManager : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="WorldSim|Person")
    void ActivateNearby(int32 RegionId, int32 MaxCount = 64);

    UFUNCTION(BlueprintCallable, Category="WorldSim|Person")
    void Simulate(float DeltaSeconds);

    UFUNCTION(BlueprintCallable, Category="WorldSim|Person")
    void DeactivateOutOfRange(const TArray<FGuid>& VisibleNow);

    UFUNCTION(BlueprintCallable, Category="WorldSim|Person")
    void SpawnOrUpdatePersonFull(FGuid PersonId, const FPersonLiteState& Lite);

    UFUNCTION(BlueprintCallable, Category="WorldSim|Person")
    FPersonFullState GetPersonFullState(FGuid PersonId) const;

private:
    UPROPERTY()
    TMap<FGuid, FPersonFullState> ActivePersonStates;

    UPROPERTY()
    TSet<FGuid> VisiblePersonIds;
};
```

```cpp
#include "LocalPersonManager.h"

void ULocalPersonManager::ActivateNearby(int32 RegionId, int32 MaxCount)
{
    // TODO: 从 Presence/Commitment 获取候选，按区间和优先级实例化 PersonFull
    UE_LOG(LogTemp, Log, TEXT("ActivateNearby Region=%d Max=%d"), RegionId, MaxCount);
}

void ULocalPersonManager::Simulate(float DeltaSeconds)
{
    // TODO: 局部行为 tick（仅对 ActivePersonStates）
    for (auto& Pair : ActivePersonStates)
    {
        Pair.Value.EmotionalVec.X = FMath::Clamp(Pair.Value.EmotionalVec.X + DeltaSeconds * 0.01f, -1.0f, 1.0f);
    }
}

void ULocalPersonManager::DeactivateOutOfRange(const TArray<FGuid>& VisibleNow)
{
    TSet<FGuid> KeepSet(VisibleNow);
    for (const auto& Pair : ActivePersonStates)
    {
        if (!KeepSet.Contains(Pair.Key))
        {
            VisiblePersonIds.Remove(Pair.Key);
        }
    }
}

void ULocalPersonManager::SpawnOrUpdatePersonFull(FGuid PersonId, const FPersonLiteState& Lite)
{
    FPersonFullState Full;
    Full.PersonId = Lite.PersonId;
    Full.RegionCellId = Lite.HomeRegionId;
    Full.ActivityState = EPersonActivityState::Offline;
    Full.Health = Lite.LifeState == EExistenceState::Alive ? 1.0f : 0.0f;
    ActivePersonStates.Add(PersonId, Full);
    VisiblePersonIds.Add(PersonId);
}

FPersonFullState ULocalPersonManager::GetPersonFullState(FGuid PersonId) const
{
    if (const FPersonFullState* Found = ActivePersonStates.Find(PersonId))
    {
        return *Found;
    }
    return FPersonFullState();
}
```

---

## 8) `PlayerBirthService.h/.cpp`（阶段3）
```cpp
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "WorldSimTypes.h"
#include "PlayerBirthService.generated.h"

USTRUCT(BlueprintType)
struct FPlayerCharacterProfile
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FGuid PlayerId;

    UPROPERTY(BlueprintReadOnly)
    FPlayerCharacterInput Input;

    UPROPERTY(BlueprintReadOnly)
    int32 BirthRegionId = INDEX_NONE;

    UPROPERTY(BlueprintReadOnly)
    FPersonLiteState Lite;
};

UCLASS()
class WORLDSIMDEMO_API UPlayerBirthService : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="WorldSim|Player")
    FGuid CreatePlayerProfile(const FPlayerCharacterInput& Input);

    UFUNCTION(BlueprintCallable, Category="WorldSim|Player")
    int32 SelectBirthRegion(const FPlayerCharacterInput& Input);

    UFUNCTION(BlueprintCallable, Category="WorldSim|Player")
    FPlayerCharacterProfile GetProfile(FGuid PlayerId) const;

private:
    UPROPERTY()
    TMap<FGuid, FPlayerCharacterProfile> Profiles;
};
```

```cpp
#include "PlayerBirthService.h"

FGuid UPlayerBirthService::CreatePlayerProfile(const FPlayerCharacterInput& Input)
{
    FPlayerCharacterProfile NewProfile;
    NewProfile.PlayerId = FGuid::NewGuid();
    NewProfile.Input = Input;
    NewProfile.BirthRegionId = SelectBirthRegion(Input);

    NewProfile.Lite.PersonId = NewProfile.PlayerId;
    NewProfile.Lite.HomeRegionId = NewProfile.BirthRegionId;
    NewProfile.Lite.LifeState = EExistenceState::Alive;
    NewProfile.Lite.OccupationCode = TEXT("Civilian");

    Profiles.Add(NewProfile.PlayerId, NewProfile);
    return NewProfile.PlayerId;
}

int32 UPlayerBirthService::SelectBirthRegion(const FPlayerCharacterInput& Input)
{
    // 简化版本：基于输入返回一个稳定结果。
    return (GetTypeHash(Input.AppearanceSeed) + Input.CulturalPreference * 31 + Input.CareerInterest * 7) % 10;
}

FPlayerCharacterProfile UPlayerBirthService::GetProfile(FGuid PlayerId) const
{
    if (const FPlayerCharacterProfile* Found = Profiles.Find(PlayerId))
    {
        return *Found;
    }
    return FPlayerCharacterProfile();
}
```

---

## 9) `DailyRoutineSystem.h/.cpp`（阶段3）
```cpp
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "WorldSimTypes.h"
#include "DailyRoutineSystem.generated.h"

UCLASS()
class WORLDSIMDEMO_API UDailyRoutineSystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="WorldSim|Routine")
    TArray<FPersonFullState> GenerateDailySchedule(FGuid PersonId, const FDateTime& Day);

    UFUNCTION(BlueprintCallable, Category="WorldSim|Routine")
    void ApplyDailyInterrupts(FGuid PersonId, const FRegionSnapshot& Snapshot);

private:
    UPROPERTY()
    TMap<FGuid, TArray<FPersonFullState>> DailyPlans;
};
```

```cpp
#include "DailyRoutineSystem.h"

TArray<FPersonFullState> UDailyRoutineSystem::GenerateDailySchedule(FGuid PersonId, const FDateTime& Day)
{
    TArray<FPersonFullState> States;
    // 最小示例：一天2个时段动作（可扩展为完整状态机）
    FPersonFullState Eat;
    Eat.PersonId = PersonId;
    Eat.ActivityState = EPersonActivityState::InOperation;
    Eat.NextPlanTime = Day;
    Eat.ActiveTags.Add(TEXT("Breakfast"));

    FPersonFullState Work;
    Work.PersonId = PersonId;
    Work.ActivityState = EPersonActivityState::Committed;
    Work.NextPlanTime = Day + FTimespan::FromHours(2);
    Work.ActiveTags.Add(TEXT("Work"));

    FPersonFullState Rest;
    Rest.PersonId = PersonId;
    Rest.ActivityState = EPersonActivityState::Offline;
    Rest.NextPlanTime = Day + FTimespan::FromHours(12);
    Rest.ActiveTags.Add(TEXT("Rest"));

    States = {Eat, Work, Rest};
    DailyPlans.Add(PersonId, States);
    return States;
}

void UDailyRoutineSystem::ApplyDailyInterrupts(FGuid PersonId, const FRegionSnapshot& Snapshot)
{
    // TODO: 天气/通勤拥堵/封锁时对日程进行偏移
    if (Snapshot.TransportLoad > 0.8f)
    {
        UE_LOG(LogTemp, Warning, TEXT("Region %d congestion, schedule of %s delayed"), Snapshot.RegionId, *PersonId.ToString());
    }
}
```

---

## 10) 蓝图调用入口（阶段1验收最小）

### `WorldSimBlueprintFunctionLibrary.h/.cpp`
```cpp
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WorldSimTypes.h"
#include "WorldTimeSubsystem.h"
#include "RegionSnapshotSubsystem.h"
#include "PresenceSubsystem.h"
#include "TruthLedgerSubsystem.h"
#include "WorldSimBlueprintFunctionLibrary.generated.h"

UCLASS()
class WORLDSIMDEMO_API UWorldSimBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category="WorldSim|API")
    static FDateTime BS_GetWorldNow(const UWorld* WorldContextObject);

    UFUNCTION(BlueprintPure, Category="WorldSim|API")
    static FRegionSnapshot BS_GetRegionSnapshot(const UWorld* WorldContextObject, int32 RegionId);

    UFUNCTION(BlueprintPure, Category="WorldSim|API")
    static bool BS_CanNpcExistHere(const UWorld* WorldContextObject, FGuid PersonId, int32 RegionId);

    UFUNCTION(BlueprintPure, Category="WorldSim|API")
    static EExistenceState BS_QueryPersonTruth(const UWorld* WorldContextObject, FGuid PersonId);
};
```

```cpp
#include "WorldSimBlueprintFunctionLibrary.h"

FDateTime UWorldSimBlueprintFunctionLibrary::BS_GetWorldNow(const UWorld* WorldContextObject)
{
    if (!WorldContextObject) return FDateTime::Now();
    if (auto* TimeSub = WorldContextObject->GetSubsystem<UWorldTimeSubsystem>())
    {
        return TimeSub->GetNow();
    }
    return FDateTime::Now();
}

FRegionSnapshot UWorldSimBlueprintFunctionLibrary::BS_GetRegionSnapshot(const UWorld* WorldContextObject, int32 RegionId)
{
    if (!WorldContextObject) return FRegionSnapshot();
    if (auto* RegionSub = WorldContextObject->GetSubsystem<URegionSnapshotSubsystem>())
    {
        return RegionSub->GetSnapshot(RegionId);
    }
    return FRegionSnapshot();
}

bool UWorldSimBlueprintFunctionLibrary::BS_CanNpcExistHere(const UWorld* WorldContextObject, FGuid PersonId, int32 RegionId)
{
    if (!WorldContextObject) return true;
    if (auto* PresSub = WorldContextObject->GetSubsystem<UPresenceSubsystem>())
    {
        return PresSub->CanAppear(PersonId, RegionId, FDateTime::UtcNow());
    }
    return true;
}

EExistenceState UWorldSimBlueprintFunctionLibrary::BS_QueryPersonTruth(const UWorld* WorldContextObject, FGuid PersonId)
{
    if (!WorldContextObject) return EExistenceState::Unknown;
    if (auto* TruthSub = WorldContextObject->GetSubsystem<UTruthLedgerSubsystem>())
    {
        EExistenceState Out = EExistenceState::Unknown;
        FWorldEvent E;
        TruthSub->TryGetLatestState(PersonId, Out, E);
        return Out;
    }
    return EExistenceState::Unknown;
}
```

---

## 11) Build.cs 关键依赖
在你的模块 `.Build.cs` 的 `PublicDependencyModuleNames` 至少包含：
- `Core`
- `CoreUObject`
- `Engine`
- `Json`
- `JsonUtilities`

---

## 12) 阶段1-3接地执行顺序

### Day 1
- 建立工程/模块
- 放入 `WorldSimTypes.h`
- 放入 `WorldTimeSubsystem`
- 编译（先不急于运行）

### Day 2
- 放入 Region/Commitment/Presence/Truth 三个子系统
- 蓝图库函数里加时间+快照+可出现查询
- 做一次“编辑器按钮触发：查询时间/快照/真相”的测试

### Day 3
- 放入 LocalPersonManager
- 放入 Birth + DailyRoutine
- 构造 1 个角色 -> 1天日程 -> 1条承诺 -> 1条日志闭环

---

> 下一步建议：你把上面的 12~13 个文件按项目名替换 `WORLDSIMDEMO_API` 与 include 路径，然后从 `Day 1` 开始贴入并编译。
