# Day1 可直接贴入 UE5 的代码（最小骨架）

目标：先保真“世界时间 + 蓝图可读接口 + 基础类型”，完成阶段1 Day1 的可编译闭环。

以下文件中 `WORLDSIMDEMO_API` 需要替换成你项目的模块导出宏（例如 `WORLD_SIM_DEMO_API`）。

---

## 1. `Source/WorldSimDemo/Public/WorldSimCoreTypes.h`

```cpp
#pragma once

#include "CoreMinimal.h"
#include "WorldSimCoreTypes.generated.h"

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
    Commuting UMETA(DisplayName = "Commuting"),
    Working UMETA(DisplayName = "Working"),
    InTransit UMETA(DisplayName = "InTransit"),
    Resting UMETA(DisplayName = "Resting"),
    Deployed UMETA(DisplayName = "Deployed"),
    Unavailable UMETA(DisplayName = "Unavailable")
};

UENUM(BlueprintType)
enum class ECommitmentType : uint8
{
    WorkShift UMETA(DisplayName = "WorkShift"),
    Transit UMETA(DisplayName = "Transit"),
    Military UMETA(DisplayName = "Military"),
    Tourism UMETA(DisplayName = "Tourism"),
    BusinessTrip UMETA(DisplayName = "BusinessTrip")
};

UENUM(BlueprintType)
enum class ECommitmentState : uint8
{
    Planned UMETA(DisplayName = "Planned"),
    Locked UMETA(DisplayName = "Locked"),
    Executing UMETA(DisplayName = "Executing"),
    Completed UMETA(DisplayName = "Completed"),
    Cancelled UMETA(DisplayName = "Cancelled"),
    Diverted UMETA(DisplayName = "Diverted"),
    Failed UMETA(DisplayName = "Failed")
};

USTRUCT(BlueprintType)
struct FPresenceInterval
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FGuid PersonId;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FDateTime StartTime = FDateTime::MinValue();

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FDateTime EndTime = FDateTime::MaxValue();

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    int32 RegionId = INDEX_NONE;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    EPersonActivityState ActivityTag = EPersonActivityState::Idle;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FGuid CommitmentId;
};

USTRUCT(BlueprintType)
struct FPersonLiteState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FGuid PersonId;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    int32 HomeRegionId = INDEX_NONE;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    EExistenceState LifeState = EExistenceState::Unknown;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FDateTime LockedUntil = FDateTime::MinValue();

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TArray<FGuid> ActiveCommitmentIds;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    int32 WealthBand = 0;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FString OccupationCode;
};

USTRUCT(BlueprintType)
struct FPersonFullState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FGuid PersonId;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    int32 RegionCellId = INDEX_NONE;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    EPersonActivityState ActivityState = EPersonActivityState::Idle;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FDateTime NextPlanTime = FDateTime::MinValue();

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FVector2D EmotionalVec = FVector2D::ZeroVector;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    float Health = 1.0f;
};

USTRUCT(BlueprintType)
struct FRegionSnapshot
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    int32 RegionId = INDEX_NONE;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FDateTime WindowStart = FDateTime::MinValue();

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    int32 Population = 0;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    float FoodPrice = 1.0f;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    float TransportLoad = 0.0f;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    float Security = 1.0f;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    float EconomyPressure = 0.0f;
};

USTRUCT(BlueprintType)
struct FCommitmentRecord
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FGuid CommitmentId;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FGuid PersonId;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    ECommitmentType Type = ECommitmentType::WorkShift;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    ECommitmentState State = ECommitmentState::Planned;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FDateTime EarliestStart = FDateTime::MinValue();

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FDateTime LatestStart = FDateTime::MinValue();

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FDateTime ExpectedEnd = FDateTime::MinValue();

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    int32 FromLocationId = INDEX_NONE;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    int32 ToLocationId = INDEX_NONE;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    bool bHardCommit = false;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    bool bCancelable = true;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    bool bExecuted = false;
};

USTRUCT(BlueprintType)
struct FWorldEvent
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FGuid EventId;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FGuid SourceCommitmentId;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FGuid PersonId;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FName EventType = NAME_None;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FDateTime EventTime = FDateTime::MinValue();

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    int32 LocationId = INDEX_NONE;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FString PayloadJson;
};

USTRUCT(BlueprintType)
struct FPlayerIdentityInput
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FString Gender;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FString AppearanceSeed;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    int32 CulturalPreference = 0;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    int32 CareerInterest = 0;
};

USTRUCT(BlueprintType)
struct FPlayerIdentityProfile
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
    FGuid PlayerId;

    UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
    int32 BirthRegionId = INDEX_NONE;

    UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
    FPlayerIdentityInput Input;

    UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
    FPersonLiteState Lite;
};
```

---

## 2. `Source/WorldSimDemo/Public/WorldTimeSubsystem.h`

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WorldTimeSubsystem.generated.h"

UCLASS()
class WORLDSIMDEMO_API UWorldTimeSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintPure, Category = "WorldSim|Time")
    FDateTime GetNow() const;

    UFUNCTION(BlueprintPure, Category = "WorldSim|Time")
    float GetScale() const;

    UFUNCTION(BlueprintCallable, Category = "WorldSim|Time")
    void SetScale(float NewScale);

    UFUNCTION(BlueprintCallable, Category = "WorldSim|Time")
    void SetNow(const FDateTime& NewNow);

    UFUNCTION(BlueprintCallable, Category = "WorldSim|Time")
    void ResetNow(const FDateTime& NewStart);

    UFUNCTION(BlueprintCallable, Category = "WorldSim|Time")
    void AdvanceWorldTime(float DeltaSeconds);

    UFUNCTION(BlueprintPure, Category = "WorldSim|Time")
    int64 GetTickIndex() const;

private:
    UPROPERTY()
    FDateTime StartTime = FDateTime::UtcNow();

    UPROPERTY()
    FDateTime CurrentTime = FDateTime::UtcNow();

    UPROPERTY()
    float TimeScale = 60.0f;   // 现实1秒=世界1分钟

    UPROPERTY()
    int64 TickIndex = 0;
};
```

## `Source/WorldSimDemo/Private/WorldTimeSubsystem.cpp`

```cpp
#include "WorldTimeSubsystem.h"

void UWorldTimeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    StartTime = FDateTime::UtcNow();
    CurrentTime = StartTime;
}

FDateTime UWorldTimeSubsystem::GetNow() const
{
    return CurrentTime;
}

float UWorldTimeSubsystem::GetScale() const
{
    return TimeScale;
}

void UWorldTimeSubsystem::SetScale(float NewScale)
{
    TimeScale = FMath::Max(0.1f, NewScale);
}

void UWorldTimeSubsystem::SetNow(const FDateTime& NewNow)
{
    CurrentTime = NewNow;
}

void UWorldTimeSubsystem::ResetNow(const FDateTime& NewStart)
{
    StartTime = NewStart;
    CurrentTime = NewStart;
}

void UWorldTimeSubsystem::AdvanceWorldTime(float DeltaSeconds)
{
    if (DeltaSeconds <= 0.0f || TimeScale <= 0.0f)
    {
        return;
    }

    CurrentTime += FTimespan::FromSeconds(static_cast<double>(DeltaSeconds * TimeScale));
    ++TickIndex;
}

int64 UWorldTimeSubsystem::GetTickIndex() const
{
    return TickIndex;
}
```

---

## 3. `Source/WorldSimDemo/Public/WorldSimBlueprintFunctionLibrary.h`

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WorldSimBlueprintFunctionLibrary.generated.h"

class UWorldTimeSubsystem;

UCLASS()
class WORLDSIMDEMO_API UWorldSimBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category = "WorldSim|API", meta = (WorldContext = "WorldContextObject"))
    static FDateTime BS_GetWorldNow(const UObject* WorldContextObject);

    UFUNCTION(BlueprintPure, Category = "WorldSim|API", meta = (WorldContext = "WorldContextObject"))
    static float BS_GetWorldTimeScale(const UObject* WorldContextObject);

    UFUNCTION(BlueprintCallable, Category = "WorldSim|API", meta = (WorldContext = "WorldContextObject"))
    static void BS_AdvanceWorldTime(const UObject* WorldContextObject, float DeltaSeconds);

    UFUNCTION(BlueprintCallable, Category = "WorldSim|API", meta = (WorldContext = "WorldContextObject"))
    static void BS_SetWorldTime(const UObject* WorldContextObject, const FDateTime& NewNow);
};
```

## `Source/WorldSimDemo/Private/WorldSimBlueprintFunctionLibrary.cpp`

```cpp
#include "WorldSimBlueprintFunctionLibrary.h"
#include "WorldTimeSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

static UWorld* GetWorldFromObject(const UObject* WorldContextObject)
{
    if (!WorldContextObject)
    {
        return nullptr;
    }

    if (!GEngine)
    {
        return nullptr;
    }

    return GEngine->GetWorldFromContextObjectChecked(WorldContextObject);
}

FDateTime UWorldSimBlueprintFunctionLibrary::BS_GetWorldNow(const UObject* WorldContextObject)
{
    UWorld* World = GetWorldFromObject(WorldContextObject);
    if (!World)
    {
        return FDateTime::UtcNow();
    }

    if (auto* TimeSub = World->GetSubsystem<UWorldTimeSubsystem>())
    {
        return TimeSub->GetNow();
    }

    return FDateTime::UtcNow();
}

float UWorldSimBlueprintFunctionLibrary::BS_GetWorldTimeScale(const UObject* WorldContextObject)
{
    UWorld* World = GetWorldFromObject(WorldContextObject);
    if (!World)
    {
        return 0.0f;
    }

    if (auto* TimeSub = World->GetSubsystem<UWorldTimeSubsystem>())
    {
        return TimeSub->GetScale();
    }

    return 0.0f;
}

void UWorldSimBlueprintFunctionLibrary::BS_AdvanceWorldTime(const UObject* WorldContextObject, float DeltaSeconds)
{
    UWorld* World = GetWorldFromObject(WorldContextObject);
    if (!World)
    {
        return;
    }

    if (auto* TimeSub = World->GetSubsystem<UWorldTimeSubsystem>())
    {
        TimeSub->AdvanceWorldTime(DeltaSeconds);
    }
}

void UWorldSimBlueprintFunctionLibrary::BS_SetWorldTime(const UObject* WorldContextObject, const FDateTime& NewNow)
{
    UWorld* World = GetWorldFromObject(WorldContextObject);
    if (!World)
    {
        return;
    }

    if (auto* TimeSub = World->GetSubsystem<UWorldTimeSubsystem>())
    {
        TimeSub->SetNow(NewNow);
    }
}
```

---

## 4. `Source/WorldSimDemo/Public/WorldSimDemoCharacter.h`（可选，若要在角色中手动推进）
你可以临时给测试角色加一个 `UFUNCTION`，用来按钮推动世界时间，不作为 Day1 必须项。

```cpp
// 仅示意：每帧可调用一次或由输入事件触发
void Tick(float DeltaTime)
{
    if (UWorld* World = GetWorld())
    {
        if (auto* TimeSub = World->GetSubsystem<UWorldTimeSubsystem>())
        {
            TimeSub->AdvanceWorldTime(DeltaTime);
        }
    }
}
```

---

## 5. 一次性验收清单（Day1）

1. 工程可编译通过（至少能通过编辑器 Hot Reload）。
2. 蓝图里调用 `BS_GetWorldNow` 可获取一个非零时间。
3. 蓝图里设置时间倍率 `BS_SetWorldTime` 可改时间。
4. 蓝图里每秒调用 `BS_AdvanceWorldTime` 后，`BS_GetWorldNow` 会有连续增长。

> 下一条我会贴 Day2（`RegionSnapshotSubsystem` + `TruthLedgerSubsystem`）的完整文件清单。
