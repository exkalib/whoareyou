# Day4 可直接贴入 UE5 的代码（局部NPC实例与回收）

Day3 之后，今天的重点是把“可见区域人物”从轻量 `PersonLite` 展开为可跑的 `PersonFull`。

这版先给出最小可运行骨架：  
- 按区域生成附近可见人物池（上限控制）  
- 本地局部状态机（轻量）  
- 离开范围回收到精简缓存（防内存/性能膨胀）

> 说明：Day4 仍以“逻辑一致”为先，状态机只做最简版循环（可替换为更真实行为树）。

---

## 1. `Source/WorldSimDemo/Public/LocalPersonManagerSubsystem.h`

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "WorldSimCoreTypes.h"
#include "LocalPersonManagerSubsystem.generated.h"

UCLASS()
class WORLDSIMDEMO_API ULocalPersonManagerSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "WorldSim|LocalPerson")
    void RegisterPersonLite(const FPersonLiteState& Lite);

    UFUNCTION(BlueprintCallable, Category = "WorldSim|LocalPerson")
    void RemovePersonLite(FGuid PersonId);

    UFUNCTION(BlueprintCallable, Category = "WorldSim|LocalPerson")
    void ActivateNearby(int32 RegionId, int32 MaxCount = 64);

    UFUNCTION(BlueprintCallable, Category = "WorldSim|LocalPerson")
    void DeactivateOutOfRange(const TSet<FGuid>& KeepPersonIds);

    UFUNCTION(BlueprintCallable, Category = "WorldSim|LocalPerson")
    void RequestPersonFull(FGuid PersonId, const FPersonLiteState& Lite);

    UFUNCTION(BlueprintCallable, Category = "WorldSim|LocalPerson")
    void RemovePerson(FGuid PersonId);

    UFUNCTION(BlueprintCallable, Category = "WorldSim|LocalPerson")
    void Tick(float DeltaSeconds);

    UFUNCTION(BlueprintPure, Category = "WorldSim|LocalPerson")
    FPersonFullState GetPersonFull(FGuid PersonId) const;

    UFUNCTION(BlueprintPure, Category = "WorldSim|LocalPerson")
    TArray<FGuid> GetVisiblePeopleInRegion(int32 RegionId) const;

private:
    void SpawnOrUpdateFromPresence(FGuid PersonId);

    UFUNCTION()
    bool TryMoveToNextMicroState(FPersonFullState& Full);

    UPROPERTY()
    TMap<FGuid, FPersonLiteState> LitePool;

    UPROPERTY()
    TMap<FGuid, FPersonFullState> ActivePersonStates;

    UPROPERTY()
    TSet<FGuid> VisiblePersonIds;
};
```

## `Source/WorldSimDemo/Private/LocalPersonManagerSubsystem.cpp`

```cpp
#include "LocalPersonManagerSubsystem.h"
#include "PresenceSubsystem.h"
#include "WorldTimeSubsystem.h"

void ULocalPersonManagerSubsystem::RegisterPersonLite(const FPersonLiteState& Lite)
{
    LitePool.FindOrAdd(Lite.PersonId) = Lite;
}

void ULocalPersonManagerSubsystem::RemovePersonLite(FGuid PersonId)
{
    LitePool.Remove(PersonId);
    ActivePersonStates.Remove(PersonId);
    VisiblePersonIds.Remove(PersonId);
}

void ULocalPersonManagerSubsystem::SpawnOrUpdateFromPresence(FGuid PersonId)
{
    if (!LitePool.Contains(PersonId))
    {
        FPersonLiteState Synthetic;
        Synthetic.PersonId = PersonId;
        Synthetic.HomeRegionId = INDEX_NONE;
        Synthetic.LifeState = EExistenceState::Alive;
        LitePool.Add(PersonId, Synthetic);
    }

    const FPersonLiteState& Lite = LitePool[PersonId];
    FPersonFullState& Full = ActivePersonStates.FindOrAdd(PersonId);
    Full.PersonId = Lite.PersonId;
    Full.Health = (Lite.LifeState == EExistenceState::Dead ? 0.0f : 1.0f);
    Full.RegionCellId = Lite.HomeRegionId;
    Full.EmotionalVec = FVector2D::ZeroVector;
    Full.ActivityState = Full.Health > 0.0f ? EPersonActivityState::Idle : EPersonActivityState::Unavailable;
    Full.NextPlanTime = FDateTime::UtcNow();
}

void ULocalPersonManagerSubsystem::ActivateNearby(int32 RegionId, int32 MaxCount)
{
    UGameInstance* GI = GetGameInstance();
    if (!GI) return;

    UWorld* World = GI->GetWorld();
    if (!World) return;

    auto* PresenceSub = World->GetSubsystem<UPresenceSubsystem>();
    auto* TimeSub = World->GetSubsystem<UWorldTimeSubsystem>();
    if (!PresenceSub || !TimeSub) return;

    const FDateTime Now = TimeSub->GetNow();
    TArray<FGuid> Candidates = PresenceSub->GetPeopleAt(Now, RegionId);

    int32 Added = 0;
    for (FGuid PersonId : Candidates)
    {
        if (VisiblePersonIds.Contains(PersonId)) continue;
        if (Added >= MaxCount) break;

        SpawnOrUpdateFromPresence(PersonId);
        FPersonFullState& Full = ActivePersonStates[PersonId];
        Full.RegionCellId = RegionId;
        Full.NextPlanTime = Now + FTimespan::FromMinutes(30 + Added * 3);
        VisiblePersonIds.Add(PersonId);
        Added++;
    }
}

void ULocalPersonManagerSubsystem::DeactivateOutOfRange(const TSet<FGuid>& KeepPersonIds)
{
    TArray<FGuid> ToRemove;
    for (auto& Pair : ActivePersonStates)
    {
        if (!KeepPersonIds.Contains(Pair.Key))
        {
            ToRemove.Add(Pair.Key);
        }
    }

    for (FGuid Id : ToRemove)
    {
        ActivePersonStates.Remove(Id);
        VisiblePersonIds.Remove(Id);
    }
}

void ULocalPersonManagerSubsystem::RequestPersonFull(FGuid PersonId, const FPersonLiteState& Lite)
{
    LitePool.Add(PersonId, Lite);
    SpawnOrUpdateFromPresence(PersonId);
    VisiblePersonIds.Add(PersonId);
}

void ULocalPersonManagerSubsystem::RemovePerson(FGuid PersonId)
{
    ActivePersonStates.Remove(PersonId);
    VisiblePersonIds.Remove(PersonId);
}

bool ULocalPersonManagerSubsystem::TryMoveToNextMicroState(FPersonFullState& Full)
{
    if (Full.Health <= 0.0f)
    {
        Full.ActivityState = EPersonActivityState::Unavailable;
        return false;
    }

    switch (Full.ActivityState)
    {
    case EPersonActivityState::Idle:
        Full.ActivityState = EPersonActivityState::Commuting;
        break;
    case EPersonActivityState::Commuting:
        Full.ActivityState = EPersonActivityState::Working;
        break;
    case EPersonActivityState::Working:
        Full.ActivityState = EPersonActivityState::Resting;
        break;
    case EPersonActivityState::Resting:
        Full.ActivityState = EPersonActivityState::Idle;
        break;
    default:
        Full.ActivityState = EPersonActivityState::Idle;
        break;
    }

    return true;
}

void ULocalPersonManagerSubsystem::Tick(float DeltaSeconds)
{
    if (DeltaSeconds <= 0.0f)
    {
        return;
    }

    UGameInstance* GI = GetGameInstance();
    if (!GI) return;

    UWorld* World = GI->GetWorld();
    if (!World) return;

    auto* TimeSub = World->GetSubsystem<UWorldTimeSubsystem>();
    if (!TimeSub) return;

    const FDateTime Now = TimeSub->GetNow();
    for (auto& Pair : ActivePersonStates)
    {
        FPersonFullState& Full = Pair.Value;

        // 简单情绪波动，防止静止
        const float Noise = FMath::Sin(static_cast<float>(Now.ToUnixTimestamp()) * 0.0001f + Pair.Key.A);
        Full.EmotionalVec.X = FMath::Clamp(Noise, -1.0f, 1.0f);
        Full.EmotionalVec.Y = FMath::Clamp(-Noise, -1.0f, 1.0f);

        if (Full.Health <= 0.0f)
        {
            Full.ActivityState = EPersonActivityState::Unavailable;
            continue;
        }

        if (Now >= Full.NextPlanTime)
        {
            if (TryMoveToNextMicroState(Full))
            {
                const int32 NextMinutes = 5 + (Pair.Key.A & 63);
                Full.NextPlanTime = Now + FTimespan::FromMinutes(NextMinutes);
            }
        }
    }
}

FPersonFullState ULocalPersonManagerSubsystem::GetPersonFull(FGuid PersonId) const
{
    if (const FPersonFullState* Full = ActivePersonStates.Find(PersonId))
    {
        return *Full;
    }
    return FPersonFullState();
}

TArray<FGuid> ULocalPersonManagerSubsystem::GetVisiblePeopleInRegion(int32 RegionId) const
{
    TArray<FGuid> Out;
    for (const auto& Pair : ActivePersonStates)
    {
        if (Pair.Value.RegionCellId == RegionId)
        {
            Out.Add(Pair.Key);
        }
    }
    return Out;
}
```

---

## 2. （可选）在 Player/GameInstance 中按秒调用 Tick（最小驱动）

你可以在某个全局系统里每帧/每秒调用：

```cpp
if (auto* LocalPersonSub = GetGameInstance()->GetSubsystem<ULocalPersonManagerSubsystem>())
{
    LocalPersonSub->Tick(DeltaSeconds);
}
```

> 强烈建议把 `DeltaSeconds` 缩小到“1~2秒虚拟秒一次”用于稳定。否则大量人物状态切换会快速抖动。

---

## 3. Day4 验收（必须）

1. 在 Presence 有若干同区人物时，`ActivateNearby` 后 `GetVisiblePeopleInRegion` 返回数量 >0 且 <= MaxCount。
2. `Tick` 可推进局部状态（Idle->Commuting->Working->Resting 循环）。
3. `DeactivateOutOfRange` 移出不在 Keep 列表的可见者。
4. `RequestPersonFull` 后可立刻读到 `FPersonFullState`。
5. `RemovePerson`/`RemovePersonLite` 后对象不再返回。

完成后你就有了阶段4最小可跑环节。下一步我给你：  
- Stage3 的“角色出生 + 日常”代码包；  
- 并加一个最小“邻近交互触发器”（对话触发可变更承诺）。
