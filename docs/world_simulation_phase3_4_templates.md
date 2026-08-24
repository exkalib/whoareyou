# 阶段3~4实现模板（角色出生/日常、局部NPC实例与回收）

> 说明：请配合前面已写的 `world_simulation_*` 文档一起用。
> 这里补的是“能进度执行”的最小可用骨架，不强求系统最完整，但每一层都能对齐你的目标：
> `可复现世界`、`按需实例化`、`无穿帮`。

## 1) 阶段3扩展结构体（`WorldSimTypes.h`）

在现有基础上补充以下定义：

```cpp
// 需要的前置 include
// #include "Engine/EngineTypes.h"

UENUM(BlueprintType)
enum class EProfessionBand : uint8
{
    Unskilled UMETA(DisplayName = "Unskilled"),
    Skilled UMETA(DisplayName = "Skilled"),
    Professional UMETA(DisplayName = "Professional"),
    Command UMETA(DisplayName = "Command")
};

USTRUCT(BlueprintType)
struct FPersonProfileSeed
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    int32 CulturalAffinity = 0;

    UPROPERTY(BlueprintReadWrite)
    int32 SocialMobility = 0;

    UPROPERTY(BlueprintReadWrite)
    int32 Education = 0;

    UPROPERTY(BlueprintReadWrite)
    float WealthIndex = 0.5f;

    UPROPERTY(BlueprintReadWrite)
    EProfessionBand ProfessionBand = EProfessionBand::Skilled;

    UPROPERTY(BlueprintReadWrite)
    int32 RiskTolerance = 50; // 0~100
};

USTRUCT(BlueprintType)
struct FPersonLiteState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FGuid PersonId;

    UPROPERTY(BlueprintReadWrite)
    FString DisplayName;

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
    FPersonProfileSeed Profile;

    UPROPERTY(BlueprintReadWrite)
    TArray<FGuid> ActiveCommitmentIds;

    UPROPERTY(BlueprintReadWrite)
    float HomeAffinity = 0.5f;

    UPROPERTY(BlueprintReadWrite)
    FVector LastKnownRegionPosition = FVector::ZeroVector;
};

USTRUCT(BlueprintType)
struct FDailyScheduleStep
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FName Tag; // Breakfast / Commute / Work / Meal / Rest / Sleep

    UPROPERTY(BlueprintReadWrite)
    FTimespan StartOffset = FTimespan::FromHours(0);

    UPROPERTY(BlueprintReadWrite)
    FTimespan Duration = FTimespan::FromHours(1);

    UPROPERTY(BlueprintReadWrite)
    int32 TargetRegionId = INDEX_NONE;

    UPROPERTY(BlueprintReadWrite)
    float Priority = 1.0f;
};

USTRUCT(BlueprintType)
struct FDailyRoutine
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FDateTime Day;

    UPROPERTY(BlueprintReadWrite)
    TArray<FDailyScheduleStep> Steps;

    UPROPERTY(BlueprintReadOnly)
    int32 Version = 1;
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
    FDateTime NextPlanTime = FDateTime::UtcNow();

    UPROPERTY(BlueprintReadWrite)
    TArray<FString> ActiveTags;

    UPROPERTY(BlueprintReadWrite)
    FVector2D EmotionalVec = FVector2D::ZeroVector;

    UPROPERTY(BlueprintReadWrite)
    float Health = 1.0f;

    UPROPERTY(BlueprintReadWrite)
    int32 RegionCellId = INDEX_NONE;

    UPROPERTY(BlueprintReadWrite)
    FDailyScheduleStep CurrentStep;

    UPROPERTY(BlueprintReadWrite)
    float ProgressInStep = 0.0f;

    UPROPERTY(BlueprintReadWrite)
    FDailyRoutine TodayRoutine;
};
```

---

## 2) 阶段3：角色出生服务（`PlayerIdentityService`）

### `PlayerIdentityService.h`
```cpp
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "WorldSimTypes.h"
#include "PlayerIdentityService.generated.h"

USTRUCT(BlueprintType)
struct FCivilContext
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FString FamilyName;

    UPROPERTY(BlueprintReadWrite)
    FString GivenName;

    UPROPERTY(BlueprintReadWrite)
    int32 NationId = 0;

    UPROPERTY(BlueprintReadWrite)
    FString RegionName;
};

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
    FString CharacterName;

    UPROPERTY(BlueprintReadOnly)
    FPersonLiteState Lite;

    UPROPERTY(BlueprintReadOnly)
    FDateTime LastDayGenerated = FDateTime::UtcNow();
};

USTRUCT(BlueprintType)
struct FPlayerIdentityWeights
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float RegionAffinity = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float WageWeight = 0.2f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float CultureMatchWeight = 0.2f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float CongestionPenalty = 0.1f;
};

UCLASS()
class WORLDSIMDEMO_API UPlayerIdentityService : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="WorldSim|Identity")
    FGuid CreatePlayerProfile(const FPlayerCharacterInput& Input, int32 PreferredRegion = INDEX_NONE);

    UFUNCTION(BlueprintCallable, Category="WorldSim|Identity")
    int32 SelectBirthRegion(const FPlayerCharacterInput& Input, int32 PreferredRegion = INDEX_NONE) const;

    UFUNCTION(BlueprintCallable, Category="WorldSim|Identity")
    FPlayerCharacterProfile GetProfile(FGuid PlayerId) const;

    UFUNCTION(BlueprintCallable, Category="WorldSim|Identity")
    FCivilContext BuildCivilContext(int32 RegionId) const;

private:
    FCivilContext MakeCivilIdentityFromSeed(int32 RegionId, int32 Seed) const;
    FString MakeDisplayName(int32 RegionId, int32 Seed) const;
    FPersonProfileSeed MakeProfileSeed(const FPlayerCharacterInput& Input) const;

    UPROPERTY()
    TMap<FGuid, FPlayerCharacterProfile> Profiles;

    UPROPERTY()
    FPlayerIdentityWeights Weights;
};
```

### `PlayerIdentityService.cpp`
```cpp
#include "PlayerIdentityService.h"

FGuid UPlayerIdentityService::CreatePlayerProfile(const FPlayerCharacterInput& Input, int32 PreferredRegion)
{
    FPlayerCharacterProfile NewProfile;
    NewProfile.PlayerId = FGuid::NewGuid();
    NewProfile.Input = Input;
    NewProfile.BirthRegionId = SelectBirthRegion(Input, PreferredRegion);
    NewProfile.CharacterName = MakeDisplayName(NewProfile.BirthRegionId, GetTypeHash(NewProfile.PlayerId));

    FCivilContext Civil = BuildCivilContext(NewProfile.BirthRegionId);
    (void)Civil;

    NewProfile.Lite.PersonId = NewProfile.PlayerId;
    NewProfile.Lite.DisplayName = NewProfile.CharacterName;
    NewProfile.Lite.HomeRegionId = NewProfile.BirthRegionId;
    NewProfile.Lite.OccupationCode = TEXT("Civilian");
    NewProfile.Lite.Profile = MakeProfileSeed(Input);
    NewProfile.Lite.LifeState = EExistenceState::Alive;
    NewProfile.Lite.HomeAffinity = 0.5f;

    Profiles.Add(NewProfile.PlayerId, NewProfile);
    return NewProfile.PlayerId;
}

int32 UPlayerIdentityService::SelectBirthRegion(const FPlayerCharacterInput& Input, int32 PreferredRegion) const
{
    // 简化版得分：稳定可复现，优先尊重玩家偏好输入
    if (PreferredRegion != INDEX_NONE)
    {
        return PreferredRegion;
    }

    const int32 RegionCount = 10;
    const int32 BaseSeed = GetTypeHash(Input.AppearanceSeed) + Input.CulturalPreference * 131 + Input.CareerInterest * 17;
    int32 Region = FMath::Abs(BaseSeed) % RegionCount;

    float Score = -1.0f;
    int32 Result = Region;

    for (int32 i = 0; i < RegionCount; ++i)
    {
        float CandidateScore = FMath::FRandRange(0.0f, 1.0f);
        // 让文化匹配稍微偏置到一定区域
        if (i == Region)
        {
            CandidateScore += Weights.CultureMatchWeight;
        }
        if (CandidateScore > Score)
        {
            Score = CandidateScore;
            Result = i;
        }
    }

    return Result;
}

FPlayerCharacterProfile UPlayerIdentityService::GetProfile(FGuid PlayerId) const
{
    if (const FPlayerCharacterProfile* Found = Profiles.Find(PlayerId))
    {
        return *Found;
    }
    return FPlayerCharacterProfile();
}

FCivilContext UPlayerIdentityService::BuildCivilContext(int32 RegionId) const
{
    FCivilContext C;
    C.NationId = RegionId % 8;
    C.RegionName = FString::Printf(TEXT("Region_%d"), RegionId);
    C.FamilyName = FString::Printf(TEXT("Region%dClan"), RegionId);
    C.GivenName = TEXT("Citizen");
    return C;
}

FCivilContext UPlayerIdentityService::BuildCivilContext(int32 RegionId, int32 Seed) const
{
    FCivilContext C = BuildCivilContext(RegionId);
    C.FamilyName = FString::Printf(TEXT("C%d"), (RegionId * 31 + Seed) % 9999);
    C.GivenName = FString::Printf(TEXT("N%d"), (Seed * 7 + RegionId) % 9999);
    return C;
}

FString UPlayerIdentityService::MakeDisplayName(int32 RegionId, int32 Seed) const
{
    const FCivilContext C = BuildCivilContext(RegionId, Seed);
    return C.FamilyName + TEXT(" ") + C.GivenName;
}

FPersonProfileSeed UPlayerIdentityService::MakeProfileSeed(const FPlayerCharacterInput& Input) const
{
    FPersonProfileSeed P;
    P.CulturalAffinity = FMath::Clamp(Input.CulturalPreference, 0, 100);
    P.Education = FMath::Clamp(Input.CareerInterest, 0, 100);
    P.SocialMobility = 50;
    P.WealthIndex = 0.4f + Input.CareerInterest * 0.01f;
    P.ProfessionBand = EProfessionBand::Skilled;
    P.RiskTolerance = 50;
    return P;
}
```

---

## 3) 阶段3：日常生成与推进（`DailyRoutineSystem` 升级）

### `DailyRoutineSystem.h`
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
    FDailyRoutine GenerateDailyRoutine(FGuid PersonId, const FDateTime& Day, const FPersonLiteState& Lite);

    UFUNCTION(BlueprintCallable, Category="WorldSim|Routine")
    bool ApplyInterrupt(FGuid PersonId, const FRegionSnapshot& Snapshot, int32 Severity = 1);

    UFUNCTION(BlueprintCallable, Category="WorldSim|Routine")
    FDailyStepResult EvalStep(FGuid PersonId, const FDateTime& Now, const FDailyRoutine& Routine, FPersonFullState& OutState);

private:
    FDateTime GetDayStart(const FDateTime& InTime) const;
    float GetCommutingDelayHours(const FRegionSnapshot& Snapshot) const;

    UPROPERTY()
    TMap<FGuid, FDailyRoutine> RoutineCache;
};

USTRUCT(BlueprintType)
struct FDailyStepResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    bool bStepChanged = false;

    UPROPERTY(BlueprintReadWrite)
    FName NewTag;

    UPROPERTY(BlueprintReadWrite)
    float StepProgress = 0.0f;

    UPROPERTY(BlueprintReadWrite)
    FVector2D EmotionDelta = FVector2D::ZeroVector;
};
```

### `DailyRoutineSystem.cpp`
```cpp
#include "DailyRoutineSystem.h"

FDailyRoutine UDailyRoutineSystem::GenerateDailyRoutine(FGuid PersonId, const FDateTime& Day, const FPersonLiteState& Lite)
{
    FDailyRoutine Routine;
    Routine.Day = GetDayStart(Day);
    Routine.Version = 1;

    int32 base = FMath::Abs(GetTypeHash(PersonId)) % 100;

    FDailyScheduleStep Breakfast{TEXT("Breakfast"), FTimespan::FromHours(7), FTimespan::FromHours(1), Lite.HomeRegionId, 1.0f};
    FDailyScheduleStep CommuteToWork{TEXT("Commute"), FTimespan::FromHours(8), FTimespan::FromHours(1), Lite.HomeRegionId + 1, 1.0f};
    FDailyScheduleStep Work{TEXT("Work"), FTimespan::FromHours(9), FTimespan::FromHours(8), Lite.HomeRegionId + 1, 1.0f + Lite.Profile.ProfessionBand * 0.1f};
    FDailyScheduleStep CommuteBack{TEXT("Commute"), FTimespan::FromHours(18), FTimespan::FromHours(1), Lite.HomeRegionId, 1.0f};
    FDailyScheduleStep Dinner{TEXT("Dinner"), FTimespan::FromHours(19), FTimespan::FromHours(1), Lite.HomeRegionId, 0.8f};
    FDailyScheduleStep Night{TEXT("Sleep"), FTimespan::FromHours(22), FTimespan::FromHours(2), Lite.HomeRegionId, 1.0f};

    Routine.Steps = {Breakfast, CommuteToWork, Work, CommuteBack, Dinner, Night};

    // 人口类型偏差，让职业与作息产生差异
    if (Lite.OccupationCode == TEXT("ShiftWorker"))
    {
        Routine.Steps[0].StartOffset = FTimespan::FromHours(6);
        Routine.Steps[2].StartOffset = FTimespan::FromHours(10);
        Routine.Steps[2].Duration = FTimespan::FromHours(10);
        Routine.Steps[4].StartOffset = FTimespan::FromHours(21);
    }

    // 仅让偶发扰动依赖种子，便于复现
    if ((base % 10) == 3)
    {
        FDailyScheduleStep LateMeal{TEXT("NightMeal"), FTimespan::FromHours(23), FTimespan::FromHours(0.5f), Lite.HomeRegionId, 0.4f};
        Routine.Steps.Add(LateMeal);
    }

    RoutineCache.Add(PersonId, Routine);
    return Routine;
}

bool UDailyRoutineSystem::ApplyInterrupt(FGuid PersonId, const FRegionSnapshot& Snapshot, int32 Severity)
{
    FDailyRoutine* Routine = RoutineCache.Find(PersonId);
    if (!Routine)
    {
        return false;
    }

    // 简单模型：拥堵增加通勤时长
    for (FDailyScheduleStep& Step : Routine->Steps)
    {
        if (Step.Tag == TEXT("Commute"))
        {
            Step.Duration += FTimespan::FromMinutes(Severity * 10 * (Snapshot.TransportLoad + 1.0f));
        }
    }
    return true;
}

FDailyStepResult UDailyRoutineSystem::EvalStep(FGuid PersonId, const FDateTime& Now, const FDailyRoutine& Routine, FPersonFullState& OutState)
{
    FDailyStepResult Result;
    const FDateTime Day0 = GetDayStart(Now);
    const FTimespan SinceMidnight = Now - Day0;

    FDailyScheduleStep Chosen = Routine.Steps.Num() > 0 ? Routine.Steps.Last() : FDailyScheduleStep();
    int32 Index = 0;

    for (int32 i = 0; i < Routine.Steps.Num(); ++i)
    {
        const FDailyScheduleStep& S = Routine.Steps[i];
        const FTimespan Start = S.StartOffset;
        const FTimespan End = S.StartOffset + S.Duration;
        if (SinceMidnight >= Start && SinceMidnight <= End)
        {
            Chosen = S;
            Index = i;
            break;
        }
    }

    const FTimespan LocalSpan = SinceMidnight - Chosen.StartOffset;
    float Progress = 0.0f;
    if (Chosen.Duration.GetTotalSeconds() > 0)
    {
        Progress = FMath::Clamp((float)(LocalSpan.GetTotalSeconds() / Chosen.Duration.GetTotalSeconds()), 0.0f, 1.0f);
    }

    if (OutState.CurrentStep.Tag != Chosen.Tag)
    {
        OutState.CurrentStep = Chosen;
        OutState.ActivityState = (Chosen.Tag == TEXT("Work") || Chosen.Tag == TEXT("Commute")) ? EPersonActivityState::InOperation : EPersonActivityState::Idle;
        OutState.ProgressInStep = Progress;
        OutState.ActiveTags = {Chosen.Tag.ToString()};
        OutState.RegionCellId = Chosen.TargetRegionId;
        Result.bStepChanged = true;
        Result.NewTag = Chosen.Tag;
    }
    else
    {
        OutState.ProgressInStep = Progress;
    }

    OutState.NextPlanTime = Day0 + (Chosen.StartOffset + FTimespan::FromSeconds(Chosen.Duration.GetTotalMilliseconds() * Progress));

    // 轻度行为反馈
    if (Chosen.Tag == TEXT("Work")) Result.EmotionDelta = FVector2D(-0.02f, -0.01f);
    else if (Chosen.Tag == TEXT("Sleep")) Result.EmotionDelta = FVector2D(0.0f, 0.03f);

    Result.StepProgress = Progress;
    return Result;
}

FDateTime UDailyRoutineSystem::GetDayStart(const FDateTime& InTime) const
{
    return FDateTime(InTime.GetYear(), InTime.GetMonth(), InTime.GetDay(), 0, 0, 0);
}

float UDailyRoutineSystem::GetCommutingDelayHours(const FRegionSnapshot& Snapshot) const
{
    return FMath::Clamp(Snapshot.TransportLoad * 0.5f, 0.0f, 3.0f);
}
```

---

## 4) 阶段4：按需实例化的人口容器（`WorldPopulationSubsystem`）

### `WorldPopulationSubsystem.h`
```cpp
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WorldSimTypes.h"
#include "WorldPopulationSubsystem.generated.h"

UCLASS()
class WORLDSIMDEMO_API UWorldPopulationSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="WorldSim|Population")
    void RegisterPersonLite(const FPersonLiteState& Person);

    UFUNCTION(BlueprintCallable, Category="WorldSim|Population")
    bool GetPersonLite(FGuid PersonId, FPersonLiteState& OutPerson) const;

    UFUNCTION(BlueprintCallable, Category="WorldSim|Population")
    void UpdatePersonLite(const FPersonLiteState& Person);

    UFUNCTION(BlueprintCallable, Category="WorldSim|Population")
    TArray<FGuid> QueryPersonIdsInRegion(int32 RegionId, int32 MaxCount = 200) const;

    UFUNCTION(BlueprintCallable, Category="WorldSim|Population")
    int32 GetActiveLiteCount() const;

private:
    UPROPERTY()
    TMap<FGuid, FPersonLiteState> People;
};
```

### `WorldPopulationSubsystem.cpp`
```cpp
#include "WorldPopulationSubsystem.h"

void UWorldPopulationSubsystem::RegisterPersonLite(const FPersonLiteState& Person)
{
    People.Add(Person.PersonId, Person);
}

bool UWorldPopulationSubsystem::GetPersonLite(FGuid PersonId, FPersonLiteState& OutPerson) const
{
    if (const FPersonLiteState* Found = People.Find(PersonId))
    {
        OutPerson = *Found;
        return true;
    }
    return false;
}

void UWorldPopulationSubsystem::UpdatePersonLite(const FPersonLiteState& Person)
{
    if (People.Contains(Person.PersonId))
    {
        People[Person.PersonId] = Person;
    }
}

TArray<FGuid> UWorldPopulationSubsystem::QueryPersonIdsInRegion(int32 RegionId, int32 MaxCount) const
{
    TArray<FGuid> Result;
    for (const auto& Pair : People)
    {
        if (Pair.Value.HomeRegionId == RegionId)
        {
            Result.Add(Pair.Key);
            if (Result.Num() >= MaxCount)
            {
                break;
            }
        }
    }
    return Result;
}

int32 UWorldPopulationSubsystem::GetActiveLiteCount() const
{
    return People.Num();
}
```

---

## 5) 阶段4：局部NPC状态机与可见池（`LocalPersonManager` 增强版）

### `LocalPersonManager.h`
```cpp
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "WorldSimTypes.h"
#include "LocalPersonManager.generated.h"

USTRUCT(BlueprintType)
struct FPersonVisibilityContext
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    int32 RegionId = INDEX_NONE;

    UPROPERTY(BlueprintReadWrite)
    FVector CameraLocation = FVector::ZeroVector;

    UPROPERTY(BlueprintReadWrite)
    float MaxDistance = 2000.0f;

    UPROPERTY(BlueprintReadWrite)
    int32 MaxPersons = 64;
};

USTRUCT(BlueprintType)
struct FPersonSimBudget
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    int32 ActiveLocalLimit = 64;

    UPROPERTY(BlueprintReadWrite)
    float SimStep = 0.25f;
};

UCLASS()
class WORLDSIMDEMO_API ULocalPersonManager : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="WorldSim|Person")
    void TickLocalPersons(float DeltaSeconds, const FPersonVisibilityContext& Visibility);

    UFUNCTION(BlueprintCallable, Category="WorldSim|Person")
    TArray<FGuid> GetVisiblePersonIds() const;

    UFUNCTION(BlueprintCallable, Category="WorldSim|Person")
    bool IsPersonActive(FGuid PersonId) const;

    UFUNCTION(BlueprintCallable, Category="WorldSim|Person")
    FPersonFullState GetFullState(FGuid PersonId) const;

private:
    void ActivateCandidates(const FPersonVisibilityContext& Visibility);
    void SimulateActive(float DeltaSeconds);
    void RecycleOutOfRange(const FPersonVisibilityContext& Visibility);
    void SpawnOne(const FPersonLiteState& Lite, const FDateTime& Now, bool bUseSeededPosition);
    bool IsInRange(const FPersonFullState& Full, const FPersonVisibilityContext& Visibility) const;

    UPROPERTY()
    TMap<FGuid, FPersonFullState> ActiveById;

    UPROPERTY()
    TSet<FGuid> VisibleNow;

    UPROPERTY()
    FPersonSimBudget Budget;
};
```

### `LocalPersonManager.cpp`
```cpp
#include "LocalPersonManager.h"
#include "WorldPopulationSubsystem.h"
#include "DailyRoutineSystem.h"
#include "PresenceSubsystem.h"

void ULocalPersonManager::TickLocalPersons(float DeltaSeconds, const FPersonVisibilityContext& Visibility)
{
    ActivateCandidates(Visibility);
    SimulateActive(DeltaSeconds);
    RecycleOutOfRange(Visibility);
}

void ULocalPersonManager::ActivateCandidates(const FPersonVisibilityContext& Visibility)
{
    if (!GetWorld()) return;

    UWorldPopulationSubsystem* PopSub = GetGameInstance()->GetSubsystem<UWorldPopulationSubsystem>();
    UPresenceSubsystem* Presence = GetWorld()->GetSubsystem<UPresenceSubsystem>();
    UDailyRoutineSystem* Routine = GetGameInstance()->GetSubsystem<UDailyRoutineSystem>();
    if (!PopSub || !Presence || !Routine)
    {
        return;
    }

    if (ActiveById.Num() >= Budget.ActiveLocalLimit)
    {
        return;
    }

    const TArray<FGuid> Candidates = PopSub->QueryPersonIdsInRegion(Visibility.RegionId, Visibility.MaxPersons * 3);
    for (const FGuid& PersonId : Candidates)
    {
        if (ActiveById.Contains(PersonId) || ActiveById.Num() >= Budget.ActiveLocalLimit)
        {
            continue;
        }

        if (Presence)
        {
            const FDateTime Now = FDateTime::UtcNow();
            if (!Presence->CanAppear(PersonId, Visibility.RegionId, Now))
            {
                continue;
            }
        }

        FPersonLiteState Lite;
        if (!PopSub->GetPersonLite(PersonId, Lite))
        {
            continue;
        }

        FPersonFullState Full;
        Full.PersonId = Lite.PersonId;
        Full.RegionCellId = Lite.HomeRegionId;
        Full.ActivityState = EPersonActivityState::Offline;
        Full.Health = (Lite.LifeState == EExistenceState::Alive) ? 1.0f : 0.0f;
        Full.TodayRoutine = Routine->GenerateDailyRoutine(PersonId, FDateTime::UtcNow(), Lite);

        // 简化：按 region 安排初始位置
        Full.WorldTransform.SetLocation(FVector(Visibility.CameraLocation.X + 100.f + ActiveById.Num() * 10.f, Visibility.CameraLocation.Y, 0.f));
        ActiveById.Add(PersonId, Full);
        VisibleNow.Add(PersonId);
    }
}

void ULocalPersonManager::SimulateActive(float DeltaSeconds)
{
    UDailyRoutineSystem* Routine = GetGameInstance()->GetSubsystem<UDailyRoutineSystem>();
    if (!Routine) return;

    const FDateTime Now = FDateTime::UtcNow();
    for (auto& Pair : ActiveById)
    {
        FPersonFullState& Full = Pair.Value;

        FDailyStepResult StepRes = Routine->EvalStep(Full.PersonId, Now, Full.TodayRoutine, Full);
        Full.EmotionalVec = Full.EmotionalVec + StepRes.EmotionDelta * DeltaSeconds * 0.1f;
        Full.EmotionalVec = Full.EmotionalVec.GetClampedToMaxSize(1.0f);

        // 缓慢靠近目标区域（可替换成 navmesh 行走）
        FVector Pos = Full.WorldTransform.GetLocation();
        FVector Target = FVector(Full.RegionCellId * 1000.0f, Full.RegionCellId * 10.0f, Pos.Z);
        Full.WorldTransform.SetLocation(FMath::VInterpTo(Pos, Target, DeltaSeconds, 0.5f));

        if (StepRes.bStepChanged)
        {
            UE_LOG(LogTemp, Log, TEXT("Person %s switched to %s"), *Full.PersonId.ToString(), *StepRes.NewTag.ToString());
        }
    }
}

void ULocalPersonManager::RecycleOutOfRange(const FPersonVisibilityContext& Visibility)
{
    TArray<FGuid> ToRemove;
    for (const auto& Pair : ActiveById)
    {
        const FGuid PersonId = Pair.Key;
        if (!IsInRange(Pair.Value, Visibility))
        {
            ToRemove.Add(PersonId);
        }
    }

    for (const FGuid& Id : ToRemove)
    {
        ActiveById.Remove(Id);
        VisibleNow.Remove(Id);
    }
}

TArray<FGuid> ULocalPersonManager::GetVisiblePersonIds() const
{
    return VisibleNow.Array();
}

bool ULocalPersonManager::IsPersonActive(FGuid PersonId) const
{
    return ActiveById.Contains(PersonId);
}

FPersonFullState ULocalPersonManager::GetFullState(FGuid PersonId) const
{
    if (const FPersonFullState* Found = ActiveById.Find(PersonId))
    {
        return *Found;
    }
    return FPersonFullState();
}

bool ULocalPersonManager::IsInRange(const FPersonFullState& Full, const FPersonVisibilityContext& Visibility) const
{
    if (Full.RegionCellId != Visibility.RegionId)
    {
        return false;
    }

    const float Dist = FVector::Dist(Full.WorldTransform.GetLocation(), Visibility.CameraLocation);
    return Dist <= Visibility.MaxDistance;
}

void ULocalPersonManager::SpawnOne(const FPersonLiteState& Lite, const FDateTime& Now, bool bUseSeededPosition)
{
    if (ActiveById.Contains(Lite.PersonId))
    {
        return;
    }

    FPersonFullState Full;
    Full.PersonId = Lite.PersonId;
    Full.RegionCellId = Lite.HomeRegionId;
    Full.ActivityState = EPersonActivityState::Offline;
    Full.Health = (Lite.LifeState == EExistenceState::Alive) ? 1.0f : 0.0f;
    ActiveById.Add(Lite.PersonId, Full);
}
```

---

## 6) 阶段3~4最小联调流程（建议你先验证这4条）

1. 通过 `UPlayerIdentityService` 创建 1 个玩家角色。
   - 获取到 `PlayerId`，检查 `BirthRegionId`、`OccupationCode`、`Profile`。
2. 用该 `PlayerId` 对应 Lite 注册到 `WorldPopulationSubsystem`。
3. 生成今天日常：`DailyRoutineSystem::GenerateDailyRoutine`。
4. 在一个区域下调用 `LocalPersonManager::TickLocalPersons`。
5. 观察该人是否在可见池中并有 `Breakfast/Work/Sleep` 行为切换。
6. 给某个NPC触发 `ConversationCommitmentBridge::OnNpcSaysHardEvent`（例如 `Tourism`）。
7. 调度 tick 后查 `TruthLedger` 是否有 `Departed/Arrived/Returned`。

---

## 7) 关键对齐回顾（阶段3~4目标）

- 阶段3（角色/日常）
  - 有输入驱动的角色出生匹配。
  - 有基础社会属性。
  - 有每日行为骨架：吃饭/通勤/工作/休息。

- 阶段4（局部实例）
  - 按区域候选 + 视域限制创建 `PersonFull`。
  - 限制同屏人数（避免全量运算）。
  - 离开视域回收，保留 Lite 真相层。

---

> 下一步：我再给你补阶段5（冲突/回退：延误/取消/改签）的小闭环模板，直接放到 `WorldSimSchedulerSubsystem` 里，能把“明明有任务但实际没到”这类情况也处理完整。
