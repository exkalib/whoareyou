# 阶段1~3：UE5可用代码包（可直接贴入工程）

这份清单基于你当前目标：**先做单星球一致性世界基底，不先做画面**。  
目录约定（按 UE5 C++ 工程）：
- `Source/WorldSimDemo/Public/`  
- `Source/WorldSimDemo/Private/`

> 先把这些文件落地，先保证“可编译骨架 + 基础一致性规则”成立，再继续阶段4（可交互NPC）和后续系统。

## 0）基础前置

### 目标工程模块名替换
本模板里统一写 `WORLDSIMDEMO_API`，你在工程里改成真实 Module API 宏（例如 `WORLD_SIM_DEMO_API`，取决于 `.Build.cs` 的模块名）。

### Build.cs 最小依赖（`Source/WorldSimDemo/WorldSimDemo.Build.cs`）
- `Core`
- `CoreUObject`
- `Engine`
- `Json`
- `JsonUtilities`

可用示例：

```csharp
PublicDependencyModuleNames.AddRange(new string[]
{
    "Core",
    "CoreUObject",
    "Engine",
    "InputCore",
    "Json",
    "JsonUtilities"
});
```

## 1）共享类型：`WorldSimCoreTypes.h`

文件：`Source/WorldSimDemo/Public/WorldSimCoreTypes.h`

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

    UPROPERTY(BlueprintReadWrite)
    FGuid PersonId;

    UPROPERTY(BlueprintReadWrite)
    FDateTime StartTime = FDateTime::MinValue();

    UPROPERTY(BlueprintReadWrite)
    FDateTime EndTime = FDateTime::MaxValue();

    UPROPERTY(BlueprintReadWrite)
    int32 RegionId = INDEX_NONE;

    UPROPERTY(BlueprintReadWrite)
    EPersonActivityState ActivityTag = EPersonActivityState::Idle;

    UPROPERTY(BlueprintReadWrite)
    FGuid CommitmentId;
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
    TArray<FGuid> ActiveCommitmentIds;

    UPROPERTY(BlueprintReadWrite)
    int32 WealthBand = 0;

    UPROPERTY(BlueprintReadWrite)
    FString OccupationCode;
};

USTRUCT(BlueprintType)
struct FPersonFullState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FGuid PersonId;

    UPROPERTY(BlueprintReadWrite)
    int32 RegionCellId = INDEX_NONE;

    UPROPERTY(BlueprintReadWrite)
    EPersonActivityState ActivityState = EPersonActivityState::Idle;

    UPROPERTY(BlueprintReadWrite)
    FDateTime NextPlanTime = FDateTime::UtcNow();

    UPROPERTY(BlueprintReadWrite)
    FVector2D EmotionalVec = FVector2D::ZeroVector;

    UPROPERTY(BlueprintReadWrite)
    float Health = 1.0f;
};

USTRUCT(BlueprintType)
struct FRegionSnapshot
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    int32 RegionId = INDEX_NONE;

    UPROPERTY(BlueprintReadWrite)
    FDateTime WindowStart = FDateTime::UtcNow();

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
};

USTRUCT(BlueprintType)
struct FCommitmentRecord
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FGuid CommitmentId;

    UPROPERTY(BlueprintReadWrite)
    FGuid PersonId;

    UPROPERTY(BlueprintReadWrite)
    ECommitmentType Type = ECommitmentType::WorkShift;

    UPROPERTY(BlueprintReadWrite)
    ECommitmentState State = ECommitmentState::Planned;

    UPROPERTY(BlueprintReadWrite)
    FDateTime EarliestStart = FDateTime::UtcNow();

    UPROPERTY(BlueprintReadWrite)
    FDateTime LatestStart = FDateTime::UtcNow();

    UPROPERTY(BlueprintReadWrite)
    FDateTime ExpectedEnd = FDateTime::UtcNow() + FTimespan::FromHours(8.0);

    UPROPERTY(BlueprintReadWrite)
    int32 FromLocationId = INDEX_NONE;

    UPROPERTY(BlueprintReadWrite)
    int32 ToLocationId = INDEX_NONE;

    UPROPERTY(BlueprintReadWrite)
    bool bHardCommit = false;

    UPROPERTY(BlueprintReadWrite)
    bool bCancelable = true;

    UPROPERTY(BlueprintReadWrite)
    bool bExecuted = false;
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
    FName EventType;

    UPROPERTY(BlueprintReadWrite)
    FDateTime EventTime = FDateTime::UtcNow();

    UPROPERTY(BlueprintReadWrite)
    int32 LocationId = INDEX_NONE;

    UPROPERTY(BlueprintReadWrite)
    FString PayloadJson;
};

USTRUCT(BlueprintType)
struct FPlayerIdentityInput
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

USTRUCT(BlueprintType)
struct FPlayerIdentityProfile
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FGuid PlayerId;

    UPROPERTY(BlueprintReadOnly)
    int32 BirthRegionId = INDEX_NONE;

    UPROPERTY(BlueprintReadOnly)
    FPlayerIdentityInput Input;

    UPROPERTY(BlueprintReadOnly)
    FPersonLiteState Lite;
};
```

---

## 2）阶段1 核心系统

### 2.1 世界时间 `WorldTimeSubsystem`
- 文件：
  - `Source/WorldSimDemo/Public/WorldTimeSubsystem.h`
  - `Source/WorldSimDemo/Private/WorldTimeSubsystem.cpp`
- 核心接口：
  - `FDateTime GetNow() const`
  - `void SetScale(float)`
  - `float GetScale() const`
  - `void AdvanceWorldTime(float DeltaSeconds)`
  - `void SetNow(const FDateTime& NewNow)`
  - `void ResetNow(const FDateTime& NewStart)`
  - `int64 GetTickIndex() const`

```cpp
// WorldTimeSubsystem.h
#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WorldSimCoreTypes.h"
#include "WorldTimeSubsystem.generated.h"

UCLASS()
class WORLDSIMDEMO_API UWorldTimeSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintPure, Category="WorldSim|Time")
    FDateTime GetNow() const;

    UFUNCTION(BlueprintPure, Category="WorldSim|Time")
    float GetScale() const;

    UFUNCTION(BlueprintCallable, Category="WorldSim|Time")
    void SetScale(float NewScale);

    UFUNCTION(BlueprintCallable, Category="WorldSim|Time")
    void AdvanceWorldTime(float DeltaSeconds);

    UFUNCTION(BlueprintCallable, Category="WorldSim|Time")
    void SetNow(const FDateTime& NewNow);

    UFUNCTION(BlueprintCallable, Category="WorldSim|Time")
    void ResetNow(const FDateTime& NewStart);

    UFUNCTION(BlueprintPure, Category="WorldSim|Time")
    int64 GetTickIndex() const;

private:
    UPROPERTY()
    FDateTime StartTime = FDateTime::UtcNow();

    UPROPERTY()
    FDateTime CurrentTime = FDateTime::UtcNow();

    UPROPERTY()
    float TimeScale = 60.0f; // 现实秒 = 世界1分钟

    UPROPERTY()
    int64 TickIndex = 0;
};
```

```cpp
// WorldTimeSubsystem.cpp
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

float UWorldTimeSubsystem::GetScale() const { return TimeScale; }

void UWorldTimeSubsystem::SetScale(float NewScale)
{
    TimeScale = FMath::Max(0.1f, NewScale);
}

void UWorldTimeSubsystem::AdvanceWorldTime(float DeltaSeconds)
{
    CurrentTime += FTimespan::FromSeconds(DeltaSeconds * TimeScale);
    TickIndex++;
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

int64 UWorldTimeSubsystem::GetTickIndex() const { return TickIndex; }
```

### 2.2 区域快照 `RegionSnapshotSubsystem`
- 文件：
  - `Source/WorldSimDemo/Public/RegionSnapshotSubsystem.h`
  - `Source/WorldSimDemo/Private/RegionSnapshotSubsystem.cpp`
- 接口：
  - `void EnsureRegion(int32 RegionId)`
  - `FRegionSnapshot GetSnapshot(int32 RegionId) const`
  - `void SetSnapshot(const FRegionSnapshot& Snapshot)`
  - `void ApplyDelta(int32 RegionId, int32 PopulationDelta, float SecurityDelta, float EconomyDelta)`
  - `void Tick(float DeltaHours)`

### 2.3 存在性 `PresenceSubsystem`
- 文件：
  - `Source/WorldSimDemo/Public/PresenceSubsystem.h/.cpp`
- 接口：
  - `bool TryReservePresence(const FPresenceInterval& Interval, FString* RejectReason = nullptr)`
  - `void ReleasePresence(FGuid PersonId, FDateTime At)`
  - `bool CanAppearAt(FGuid PersonId, int32 RegionId, FDateTime At) const`
  - `TOptional<FPresenceInterval> GetPresence(FGuid PersonId) const`
  - `TArray<FGuid> GetPeopleAt(FDateTime At, int32 RegionId) const`
  - `void DebugSweepExpired(FDateTime Now)`

### 2.4 真相层 `TruthLedgerSubsystem`
- 文件：
  - `Source/WorldSimDemo/Public/TruthLedgerSubsystem.h/.cpp`
- 接口：
  - `void RecordEvent(const FWorldEvent& Event)`
  - `bool ResolveLatestState(FGuid PersonId, EExistenceState& OutState, FWorldEvent& OutEvidence) const`
  - `TArray<FWorldEvent> QueryEventsByPerson(FGuid PersonId, FDateTime From, FDateTime To) const`
  - `bool HasHardConflict(FGuid PersonId, int32 RegionId, FDateTime At) const`

### 2.5 承诺 `CommitmentSubsystem`
- 文件：
  - `Source/WorldSimDemo/Public/CommitmentSubsystem.h/.cpp`
- 接口：
  - `FGuid CreateCommitment(const FCommitmentRecord& In)`
  - `bool CancelCommitment(FGuid CommitmentId, const FString& Reason)`
  - `bool UpdateCommitmentState(FGuid CommitmentId, ECommitmentState NewState, const FString& Reason)`
  - `bool BindPresence(FGuid CommitmentId, const FPresenceInterval& Interval, FString* RejectReason = nullptr)`
  - `TArray<FGuid> GetCommitmentsForPerson(FGuid PersonId) const`
  - `TArray<FGuid> GetDueCommitments(FDateTime Now, float LookAheadHours = 1.0f) const`

### 2.6 本地实例管理 `LocalPersonManagerSubsystem`
- 文件：
  - `Source/WorldSimDemo/Public/LocalPersonManagerSubsystem.h/.cpp`
- 接口：
  - `void ActivateNearby(int32 RegionId, int32 MaxCount = 64)`
  - `void DeactivateOutOfRange(const TSet<FGuid>& KeepIds)`
  - `void Tick(float DeltaSeconds)`
  - `void RequestPersonFull(FGuid PersonId, const FPersonLiteState& Lite)`
  - `void RemovePerson(FGuid PersonId)`
  - `TArray<FGuid> GetVisiblePeopleInRegion(int32 RegionId) const`
  - `FPersonFullState GetPersonFull(FGuid PersonId) const`

### 2.7 蓝图入口 `WorldSimBlueprintFunctionLibrary`
- 文件：
  - `Source/WorldSimDemo/Public/WorldSimBlueprintFunctionLibrary.h/.cpp`
- 接口：
  - `FDateTime BS_GetNow(const UObject* WorldContextObject)`
  - `FRegionSnapshot BS_GetRegionSnapshot(const UObject* WorldContextObject, int32 RegionId)`
  - `bool BS_CanPersonExist(const UObject* WorldContextObject, FGuid PersonId, int32 RegionId, FDateTime At)`
  - `EExistenceState BS_QueryTruth(const UObject* WorldContextObject, FGuid PersonId, FDateTime At)`

---

## 3）阶段2 关键实现关系（最小闭环）

1. `CommitmentSubsystem::CreateCommitment` 之后，可直接创建对应 `PresenceInterval` 并通过 `PresenceSubsystem` 锁定。  
2. `PresenceSubsystem` 是单源存在检查器，不允许同一 `PersonId` 同时间存在两个位置。  
3. `TruthLedgerSubsystem` 记录 `Commitment`、到达/离开/延误/取消。  
4. `WorldSimBlueprintFunctionLibrary` 只给上层读查询，防止蓝图直接改写真相层。  

---

## 4）阶段3 角色与日常

### 4.1 玩家身份 `PlayerIdentitySubsystem`
- 文件：
  - `Source/WorldSimDemo/Public/PlayerIdentitySubsystem.h/.cpp`
- 接口：
  - `FGuid CreatePlayerProfile(const FPlayerIdentityInput& Input)`
  - `FPlayerIdentityProfile GetProfile(FGuid PlayerId) const`
  - `int32 ResolveBirthRegion(const FPlayerIdentityInput& Input)`
  - `void UpdateProfileBirthRegion(FGuid PlayerId, int32 RegionId)`
  - `void RemoveProfile(FGuid PlayerId)`

### 4.2 日常 `DailyRoutineSubsystem`
- 文件：
  - `Source/WorldSimDemo/Public/DailyRoutineSubsystem.h/.cpp`
- 接口：
  - `TArray<FPersonFullState> BuildDailySchedule(FGuid PersonId, const FDateTime& Day)`
  - `bool ApplyRegionalInterruption(FGuid PersonId, const FRegionSnapshot& Snapshot, float DeltaHours)`
  - `bool ProgressPersonRoutine(FGuid PersonId, FDateTime Now, FPersonFullState& OutNext)`
  - `bool HasNextPlan(FGuid PersonId) const`

---

## 5）阶段1-3每日清单（10天可控版本）

### Day 1（基础）
- 建工程、配置模块依赖
- 落地 `WorldSimCoreTypes.h` + `WorldTimeSubsystem`
- 验收：从蓝图可读到世界时间

### Day 2（区域）
- 落地 `RegionSnapshotSubsystem` + `TruthLedgerSubsystem`
- 验收：查一个区域快照；录入一个事件，能查该人物最新真相

### Day 3（存在性）
- 落地 `PresenceSubsystem` + 蓝图入口
- 验收：两段重叠 Presence 被拒绝；查询某人是否可在某地存在

### Day 4（承诺）
- 落地 `CommitmentSubsystem`
- 验收：创建承诺->Presence 锁存在->该时间窗内查存在一致性成立

### Day 5（压测小闭环）
- 落地冲突场景：同日“前线承诺”和“原地对话”被拒
- 验收：没有同人同刻双区存在

### Day 6（本地实例）
- 落地 `LocalPersonManagerSubsystem` + `Commitment` 关联少量人
- 验收：可在一个区域激活 30~100 人；离开回收本地状态

### Day 7（角色）
- 落地 `PlayerIdentitySubsystem`
- 验收：输入性别/外观/偏好后得到出生地和初始 `PersonLite`

### Day 8（日常）
- 落地 `DailyRoutineSubsystem`
- 验收：默认人物有“吃饭->上班->通勤->休息->夜间”循环

### Day 9（联调）
- 连接区域快照和日常中断（拥堵影响）
- 验收：拥堵上升后，日常可被延迟或改签

### Day 10（可运行最小样本）
- 建最小测试场景/蓝图UI按钮，串起：出生 -> 日程 -> 出行承诺 -> 真相查询
- 验收：逻辑闭环不自相矛盾

---

## 6）建议规则（防穿帮红线）

1. 只允许 `Commitment` 驱动的远距离移动。  
2. `PresenceSubsystem` 是全局唯一真相来源之一。  
3. 同步点：`AdvanceWorldTime` -> 承诺/Presence/Tick -> 蓝图读接口。  
4. 不直接从蓝图设置人物真相，全部由系统事件写入。  
5. 先别优化“行为真实性”，先保一致性。

---

## 7）你下一步我建议怎么做（你只要说一句“继续”）

我会按这个顺序给你贴：
1) Day1 完整 `WorldTimeSubsystem`  
2) Day2 `RegionSnapshot + TruthLedger`  
3) Day3 `Presence + BlueprintLib`  

每部分都给 **H + CPP** 一次性贴齐，直接能进工程。
