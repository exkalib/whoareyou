# 阶段3（角色出生与日常流转）可贴代码包（Day3a/Day3b）

这个包解决“我先有一个身份 -> 有出生地 -> 生成PersonLite -> 生成今天日程 -> 可以按时间推进并和本地NPC系统对接”。

目标文件：
- `Source/WorldSimDemo/Public/PlayerIdentitySubsystem.h/.cpp`
- `Source/WorldSimDemo/Public/DailyRoutineSubsystem.h/.cpp`
- `WorldSimBlueprintFunctionLibrary` 补充两个身份/日程入口（接续 Day1~Day4）

> 按你现在的工程路径把 `WORLDSIMDEMO_API` 改成项目真实 API 宏。

---

## 1) `Source/WorldSimDemo/Public/PlayerIdentitySubsystem.h`

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "WorldSimCoreTypes.h"
#include "PlayerIdentitySubsystem.generated.h"

// FPlayerIdentityProfile 已在 WorldSimCoreTypes.h 定义

UCLASS()
class WORLDSIMDEMO_API UPlayerIdentitySubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "WorldSim|Player")
    FGuid CreatePlayerProfile(const FPlayerIdentityInput& Input);

    UFUNCTION(BlueprintCallable, Category = "WorldSim|Player")
    FPlayerIdentityProfile GetProfile(FGuid PlayerId) const;

    UFUNCTION(BlueprintCallable, Category = "WorldSim|Player")
    TArray<FGuid> GetAllProfiles() const;

    UFUNCTION(BlueprintCallable, Category = "WorldSim|Player")
    int32 ResolveBirthRegion(const FPlayerIdentityInput& Input, int32 WorldSeed = 42) const;

    UFUNCTION(BlueprintCallable, Category = "WorldSim|Player")
    void SetBirthRegion(FGuid PlayerId, int32 BirthRegionId);

    UFUNCTION(BlueprintCallable, Category = "WorldSim|Player")
    void RemoveProfile(FGuid PlayerId);

private:
    UPROPERTY()
    TMap<FGuid, FPlayerIdentityProfile> Profiles;
};
```

## `Source/WorldSimDemo/Private/PlayerIdentitySubsystem.cpp`

```cpp
#include "PlayerIdentitySubsystem.h"

FGuid UPlayerIdentitySubsystem::CreatePlayerProfile(const FPlayerIdentityInput& Input)
{
    FPlayerIdentityProfile Profile;
    Profile.PlayerId = FGuid::NewGuid();
    Profile.Input = Input;
    Profile.BirthRegionId = ResolveBirthRegion(Input, 12345);

    Profile.Lite.PersonId = Profile.PlayerId;
    Profile.Lite.HomeRegionId = Profile.BirthRegionId;
    Profile.Lite.LifeState = EExistenceState::Alive;
    Profile.Lite.OccupationCode = TEXT("Civilian");
    Profile.Lite.WealthBand = FMath::Max(0, Input.CareerInterest % 5);
    Profile.Lite.ActiveCommitmentIds.Empty();

    Profiles.Add(Profile.PlayerId, Profile);
    return Profile.PlayerId;
}

FPlayerIdentityProfile UPlayerIdentitySubsystem::GetProfile(FGuid PlayerId) const
{
    if (const FPlayerIdentityProfile* Found = Profiles.Find(PlayerId))
    {
        return *Found;
    }
    return FPlayerIdentityProfile();
}

TArray<FGuid> UPlayerIdentitySubsystem::GetAllProfiles() const
{
    TArray<FGuid> Out;
    Profiles.GetKeys(Out);
    return Out;
}

int32 UPlayerIdentitySubsystem::ResolveBirthRegion(const FPlayerIdentityInput& Input, int32 WorldSeed) const
{
    // 简单而稳定的出生地选择：性别/偏好/职业偏好 + 种子 -> 0~99 区域编号
    const int32 Seed = GetTypeHash(Input.Gender) + Input.CulturalPreference * 17 + Input.CareerInterest * 31 + WorldSeed * 13;
    return FMath::Abs(Seed) % 100;
}

void UPlayerIdentitySubsystem::SetBirthRegion(FGuid PlayerId, int32 BirthRegionId)
{
    if (FPlayerIdentityProfile* Profile = Profiles.Find(PlayerId))
    {
        Profile->BirthRegionId = BirthRegionId;
        Profile->Lite.HomeRegionId = BirthRegionId;
    }
}

void UPlayerIdentitySubsystem::RemoveProfile(FGuid PlayerId)
{
    Profiles.Remove(PlayerId);
}
```

---

## 2) `Source/WorldSimDemo/Public/DailyRoutineSubsystem.h`

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "WorldSimCoreTypes.h"
#include "DailyRoutineSubsystem.generated.h"

USTRUCT(BlueprintType)
struct FRoutineSlot
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FDateTime StartTime = FDateTime::MinValue();

    UPROPERTY(BlueprintReadWrite)
    FDateTime EndTime = FDateTime::MinValue();

    UPROPERTY(BlueprintReadWrite)
    EPersonActivityState State = EPersonActivityState::Idle;

    UPROPERTY(BlueprintReadWrite)
    FString Label;
};

UCLASS()
class WORLDSIMDEMO_API UDailyRoutineSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "WorldSim|Routine")
    TArray<FRoutineSlot> BuildDailySchedule(FGuid PersonId, const FDateTime& DayStartUtc) const;

    UFUNCTION(BlueprintCallable, Category = "WorldSim|Routine")
    bool ApplyRegionalInterruption(FGuid PersonId, const FRegionSnapshot& Snapshot, float DelayMinutes);

    UFUNCTION(BlueprintCallable, Category = "WorldSim|Routine")
    bool ProgressPersonRoutine(FGuid PersonId, FDateTime Now, FPersonFullState& OutState);

    UFUNCTION(BlueprintCallable, Category = "WorldSim|Routine")
    bool HasNextPlan(FGuid PersonId) const;

private:
    FDateTime DayStart(const FDateTime& InTime) const;
    TArray<FRoutineSlot> MakeDefaultDaySlots(const FDateTime& DayStartUtc, int32 RegionId) const;

    UFUNCTION()
    void BuildDefaultIfMissing(FGuid PersonId, const FDateTime& DayStartUtc);

    UPROPERTY()
    TMap<FGuid, TArray<TArray<FRoutineSlot>>> PersonScheduleByDay; // PersonId -> schedule per day index (当天索引)

    UPROPERTY()
    TMap<FGuid, int32> ScheduleCursor;
};
```

## `Source/WorldSimDemo/Private/DailyRoutineSubsystem.cpp`

```cpp
#include "DailyRoutineSubsystem.h"

FDateTime UDailyRoutineSubsystem::DayStart(const FDateTime& InTime) const
{
    return FDateTime(InTime.GetYear(), InTime.GetMonth(), InTime.GetDay(), 0, 0, 0);
}

TArray<FRoutineSlot> UDailyRoutineSubsystem::MakeDefaultDaySlots(const FDateTime& DayStartUtc, int32 RegionId) const
{
    TArray<FRoutineSlot> Slots;

    FRoutineSlot Breakfast;
    Breakfast.StartTime = DayStartUtc + FTimespan::FromHours(7);
    Breakfast.EndTime = DayStartUtc + FTimespan::FromHours(7) + FTimespan::FromMinutes(45);
    Breakfast.State = EPersonActivityState::InTransit;
    Breakfast.Label = TEXT("早饭/出发前准备");

    FRoutineSlot Commute;
    Commute.StartTime = Breakfast.EndTime;
    Commute.EndTime = DayStartUtc + FTimespan::FromHours(9);
    Commute.State = EPersonActivityState::Commuting;
    Commute.Label = TEXT("通勤");

    FRoutineSlot Work;
    Work.StartTime = Commute.EndTime;
    Work.EndTime = DayStartUtc + FTimespan::FromHours(17);
    Work.State = EPersonActivityState::Working;
    Work.Label = TEXT("工作时段");

    FRoutineSlot Return;
    Return.StartTime = Work.EndTime;
    Return.EndTime = DayStartUtc + FTimespan::FromHours(18) + FTimespan::FromMinutes(30);
    Return.State = EPersonActivityState::Commuting;
    Return.Label = TEXT("返程");

    FRoutineSlot Night;
    Night.StartTime = Return.EndTime;
    Night.EndTime = DayStartUtc + FTimespan::FromHours(23);
    Night.State = EPersonActivityState::Resting;
    Night.Label = TEXT("晚餐/休息");

    FRoutineSlot Sleep;
    Sleep.StartTime = Night.EndTime;
    Sleep.EndTime = DayStartUtc + FTimespan::FromHours(7); // 下日7:00
    Sleep.State = EPersonActivityState::Idle;
    Sleep.Label = TEXT("夜间休息");

    Slots.Add(Breakfast);
    Slots.Add(Commute);
    Slots.Add(Work);
    Slots.Add(Return);
    Slots.Add(Night);
    Slots.Add(Sleep);
    return Slots;
}

void UDailyRoutineSubsystem::BuildDefaultIfMissing(FGuid PersonId, const FDateTime& DayStartUtc)
{
    if (!PersonScheduleByDay.Contains(PersonId))
    {
        PersonScheduleByDay.Add(PersonId, {});
    }
}

TArray<FRoutineSlot> UDailyRoutineSubsystem::BuildDailySchedule(FGuid PersonId, const FDateTime& DayStartUtc) const
{
    FDateTime DS = DayStart(DayStartUtc);
    TArray<FRoutineSlot> Slots = MakeDefaultDaySlots(DS, INDEX_NONE);

    // 本版本保持每人同日程，后续可按职业、性格、地区偏好做偏移
    return Slots;
}

bool UDailyRoutineSubsystem::ApplyRegionalInterruption(FGuid PersonId, const FRegionSnapshot& Snapshot, float DelayMinutes)
{
    if (!PersonScheduleByDay.Contains(PersonId))
    {
        return false;
    }

    if (Snapshot.TransportLoad > 0.6f || Snapshot.Security < 0.35f)
    {
        // 简化策略：按比例延迟当前活动的可见边界
        TArray<TArray<FRoutineSlot>>& PerDay = PersonScheduleByDay[PersonId];
        if (PerDay.Num() > 0)
        {
            TArray<FRoutineSlot>& Slots = PerDay.Last();
            if (Slots.Num() > 0)
            {
                for (FRoutineSlot& S : Slots)
                {
                    S.EndTime += FTimespan::FromMinutes(DelayMinutes);
                }
                return true;
            }
        }
    }
    return false;
}

bool UDailyRoutineSubsystem::ProgressPersonRoutine(FGuid PersonId, FDateTime Now, FPersonFullState& OutState)
{
    const FDateTime DS = DayStart(Now);
    BuildDefaultIfMissing(PersonId, DS);

    TArray<TArray<FRoutineSlot>>& Schedule = PersonScheduleByDay.FindOrAdd(PersonId);
    if (Schedule.Num() == 0)
    {
        Schedule.Add(BuildDailySchedule(PersonId, DS));
    }

    TArray<FRoutineSlot>& Today = Schedule.Last();
    if (Today.Num() == 0)
    {
        return false;
    }

    int32& Cursor = ScheduleCursor.FindOrAdd(PersonId, 0);
    if (Cursor < 0) Cursor = 0;
    if (Cursor >= Today.Num())
    {
        Cursor = Today.Num() - 1;
    }

    // 若已过当前段，向前推进
    while (Cursor < Today.Num() && Now >= Today[Cursor].EndTime)
    {
        Cursor++;
    }

    if (Cursor >= Today.Num())
    {
        // 回到下一天第一段
        Schedule.Add(MakeDefaultDaySlots(DS + FTimespan::FromDays(1), INDEX_NONE));
        Cursor = 0;
    }

    FRoutineSlot Current = Today[Cursor];
    OutState.ActivityState = Current.State;
    OutState.NextPlanTime = Current.EndTime;
    OutState.EmotionalVec = FVector2D(
        FMath::Clamp((Current.EndTime - Now).GetTotalSeconds() / FMath::Max(1.0, (Current.EndTime - Current.StartTime).GetTotalSeconds()), 0.0f, 1.0f),
        Cursor / FMath::Max(1.0f, Today.Num())
    );
    OutState.PersonId = PersonId;

    return true;
}

bool UDailyRoutineSubsystem::HasNextPlan(FGuid PersonId) const
{
    if (const int32* Cursor = ScheduleCursor.Find(PersonId))
    {
        if (const TArray<TArray<FRoutineSlot>>* S = PersonScheduleByDay.Find(PersonId))
        {
            if (S->Num() > 0 && S->Last().Num() > *Cursor)
            {
                return true;
            }
        }
    }
    return false;
}

```

---

## 3) 扩展 `WorldSimBlueprintFunctionLibrary`：身份和日程查询

在你的 `WorldSimBlueprintFunctionLibrary.h` 里追加：

```cpp
UFUNCTION(BlueprintCallable, Category = "WorldSim|API", meta = (WorldContext = "WorldContextObject"))
static FGuid BS_CreatePlayerProfile(const UObject* WorldContextObject, const FPlayerIdentityInput& Input);

UFUNCTION(BlueprintCallable, Category = "WorldSim|API", meta = (WorldContext = "WorldContextObject"))
static FPlayerIdentityProfile BS_GetPlayerProfile(const UObject* WorldContextObject, FGuid PlayerId);

UFUNCTION(BlueprintCallable, Category = "WorldSim|API", meta = (WorldContext = "WorldContextObject"))
static bool BS_BuildPlayerDailySchedule(const UObject* WorldContextObject, FGuid PlayerId, const FDateTime& DayStartUtc);

UFUNCTION(BlueprintCallable, Category = "WorldSim|API", meta = (WorldContext = "WorldContextObject"))
static bool BS_GetPersonStateByRoutine(const UObject* WorldContextObject, FGuid PersonId, FDateTime Now, FPersonFullState& OutState);
```

在 `WorldSimBlueprintFunctionLibrary.cpp` 里追加实现：

```cpp
#include "PlayerIdentitySubsystem.h"
#include "DailyRoutineSubsystem.h"

FGuid UWorldSimBlueprintFunctionLibrary::BS_CreatePlayerProfile(const UObject* WorldContextObject, const FPlayerIdentityInput& Input)
{
    if (!WorldContextObject)
    {
        return FGuid();
    }

    UGameInstance* GI = GEngine->GetWorldFromContextObjectChecked(WorldContextObject)->GetGameInstance();
    if (!GI)
    {
        return FGuid();
    }

    if (auto* Sub = GI->GetSubsystem<UPlayerIdentitySubsystem>())
    {
        return Sub->CreatePlayerProfile(Input);
    }
    return FGuid();
}

FPlayerIdentityProfile UWorldSimBlueprintFunctionLibrary::BS_GetPlayerProfile(const UObject* WorldContextObject, FGuid PlayerId)
{
    if (!WorldContextObject)
    {
        return FPlayerIdentityProfile();
    }

    UGameInstance* GI = GEngine->GetWorldFromContextObjectChecked(WorldContextObject)->GetGameInstance();
    if (!GI)
    {
        return FPlayerIdentityProfile();
    }

    if (auto* Sub = GI->GetSubsystem<UPlayerIdentitySubsystem>())
    {
        return Sub->GetProfile(PlayerId);
    }
    return FPlayerIdentityProfile();
}

bool UWorldSimBlueprintFunctionLibrary::BS_BuildPlayerDailySchedule(const UObject* WorldContextObject, FGuid PlayerId, const FDateTime& DayStartUtc)
{
    if (!WorldContextObject)
    {
        return false;
    }

    UGameInstance* GI = GEngine->GetWorldFromContextObjectChecked(WorldContextObject)->GetGameInstance();
    if (!GI)
    {
        return false;
    }

    auto* RoutineSub = GI->GetSubsystem<UDailyRoutineSubsystem>();
    auto* IdentitySub = GI->GetSubsystem<UPlayerIdentitySubsystem>();
    if (!RoutineSub || !IdentitySub)
    {
        return false;
    }

    const FPlayerIdentityProfile Profile = IdentitySub->GetProfile(PlayerId);
    if (!Profile.Lite.PersonId.IsValid())
    {
        return false;
    }

    RoutineSub->BuildDailySchedule(PlayerId, DayStartUtc); // 不持久写入示例中可立即返回
    return true;
}

bool UWorldSimBlueprintFunctionLibrary::BS_GetPersonStateByRoutine(const UObject* WorldContextObject, FGuid PersonId, FDateTime Now, FPersonFullState& OutState)
{
    if (!WorldContextObject)
    {
        return false;
    }

    UGameInstance* GI = GEngine->GetWorldFromContextObjectChecked(WorldContextObject)->GetGameInstance();
    if (!GI)
    {
        return false;
    }

    if (auto* RoutineSub = GI->GetSubsystem<UDailyRoutineSubsystem>())
    {
        return RoutineSub->ProgressPersonRoutine(PersonId, Now, OutState);
    }

    return false;
}
```

---

## 4) 阶段3的最小验收动作

1. 调用 `BS_CreatePlayerProfile`，返回合法 `PlayerId` 且有 `BirthRegionId`（0~99）。
2. 读取该 `FPlayerIdentityProfile`，确认 `Lite.PersonId == PlayerId` 且 `LifeState == Alive`。
3. 以 `DayStartUtc=当前世界日 00:00` 调 `BS_BuildPlayerDailySchedule` 成功。
4. 以多段 `Now` 调 `BS_GetPersonStateByRoutine`，可观察状态从通勤->工作->通勤->休息->Idle循环。
5. 区域快照拥堵高时可调用 `ApplyRegionalInterruption` 看到对应时段被延迟（可选）。

Day3 完整后，基础“玩家出生+一日自洽循环”成立。
