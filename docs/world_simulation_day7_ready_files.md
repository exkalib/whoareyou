# 阶段7：长时运行与数据生命周期（热/温/冷）最小骨架

目标：解决“玩得久了事件和快照无限涨”的问题，先做三层存储切换：
- 热层：最近 7 天（详细）
- 温层：30 天（汇总）
- 冷层：归档（摘要）

本文件给你一版可贴的最小实现（UE5 C++），先保证功能正确，不追求完美压缩率。

---

## 1) `Source/WorldSimDemo/Public/LifecycleBucketTypes.h`

```cpp
#pragma once

#include "CoreMinimal.h"
#include "WorldSimCoreTypes.h"
#include "LifecycleBucketTypes.generated.h"

USTRUCT(BlueprintType)
struct FEventDigest
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FDateTime Day = FDateTime::MinValue();

    UPROPERTY(BlueprintReadWrite)
    int32 TotalEventCount = 0;

    UPROPERTY(BlueprintReadWrite)
    int32 AliveCount = 0;

    UPROPERTY(BlueprintReadWrite)
    int32 DeadCount = 0;

    UPROPERTY(BlueprintReadWrite)
    int32 TransitCount = 0;

    UPROPERTY(BlueprintReadWrite)
    int32 ConflictCount = 0;
};

USTRUCT(BlueprintType)
struct FLifecycleConfig
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    int32 HotDays = 7;

    UPROPERTY(BlueprintReadWrite)
    int32 WarmDays = 30;

    UPROPERTY(BlueprintReadWrite)
    int32 MaxHotEvents = 100000;

    UPROPERTY(BlueprintReadWrite)
    int32 MaxWarmEvents = 300000;
};
```

---

## 2) `Source/WorldSimDemo/Public/WorldHistoryLifecycleSubsystem.h`

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "LifecycleBucketTypes.h"
#include "WorldSimCoreTypes.h"
#include "WorldHistoryLifecycleSubsystem.generated.h"

UCLASS()
class WORLDSIMDEMO_API UWorldHistoryLifecycleSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "WorldSim|Lifecycle")
    void ArchiveTick(FDateTime Now, int32 TotalWorldEvents);

    UFUNCTION(BlueprintCallable, Category = "WorldSim|Lifecycle")
    void SetConfig(const FLifecycleConfig& InConfig);

    UFUNCTION(BlueprintPure, Category = "WorldSim|Lifecycle")
    FLifecycleConfig GetConfig() const;

    UFUNCTION(BlueprintPure, Category = "WorldSim|Lifecycle")
    TArray<FEventDigest> GetDigestsInRange(FDateTime From, FDateTime To) const;

    UFUNCTION(BlueprintPure, Category = "WorldSim|Lifecycle")
    int32 GetHotEventCount() const;

    UFUNCTION(BlueprintPure, Category = "WorldSim|Lifecycle")
    int32 GetWarmEventCount() const;

private:
    void SweepHotByTime(FDateTime Now);
    void SweepWarmByCount();
    void EvictAndDigest(int32 KeepFromIndex, int32 KeepToIndex);

    UPROPERTY()
    FLifecycleConfig Config = {7, 30, 120000, 500000};

    UPROPERTY()
    TArray<FWorldEvent> HotEvents;

    UPROPERTY()
    TArray<FWorldEvent> WarmEvents;

    UPROPERTY()
    TArray<FEventDigest> ColdDigests;
};
```

## `Source/WorldSimDemo/Private/WorldHistoryLifecycleSubsystem.cpp`

```cpp
#include "WorldHistoryLifecycleSubsystem.h"
#include "Kismet/KismetMathLibrary.h"

void UWorldHistoryLifecycleSubsystem::SetConfig(const FLifecycleConfig& InConfig)
{
    Config = InConfig;
}

FLifecycleConfig UWorldHistoryLifecycleSubsystem::GetConfig() const
{
    return Config;
}

void UWorldHistoryLifecycleSubsystem::ArchiveTick(FDateTime Now, int32 TotalWorldEvents)
{
    // Step1: 时间窗清理（>= 热窗口）
    SweepHotByTime(Now);

    // Step2: 如果热量过高按数量转入温层
    SweepWarmByCount();
}

void UWorldHistoryLifecycleSubsystem::SweepHotByTime(FDateTime Now)
{
    const FDateTime HotCutoff = Now - FTimespan::FromDays(Config.HotDays);
    int32 MoveCount = 0;

    for (int32 i = 0; i < HotEvents.Num(); ++i)
    {
        if (HotEvents[i].EventTime < HotCutoff)
        {
            if (WarmEvents.Num() < Config.MaxWarmEvents)
            {
                WarmEvents.Add(HotEvents[i]);
            }
            MoveCount++;
        }
        else
        {
            // 已到边界，后面更晚的不处理
            if (i > 0)
            {
                break;
            }
        }
    }

    if (MoveCount > 0)
    {
        HotEvents.RemoveAt(0, MoveCount, /*bAllowShrinking=*/true);
    }
}

void UWorldHistoryLifecycleSubsystem::SweepWarmByCount()
{
    // 先按时间/数量粗化，超上限转成摘要
    if (WarmEvents.Num() <= Config.MaxWarmEvents) return;

    // 以天为单位聚合最早一天
    if (WarmEvents.Num() == 0) return;

    FDateTime Earliest = WarmEvents[0].EventTime;
    FDateTime DayBoundary = FDateTime(Earliest.GetYear(), Earliest.GetMonth(), Earliest.GetDay(), 0, 0, 0);
    FDateTime NextDay = DayBoundary + FTimespan::FromDays(1);
    int32 ToMove = 0;

    FEventDigest D;
    D.Day = DayBoundary;

    for (int32 i = 0; i < WarmEvents.Num(); ++i)
    {
        const FWorldEvent& E = WarmEvents[i];
        if (E.EventTime < NextDay)
        {
            D.TotalEventCount++;
            if (E.EventType == TEXT("Dead")) D.DeadCount++;
            else if (E.EventType == TEXT("Arrived")) D.TransitCount++;
            else if (E.EventType == TEXT("CommitAccepted")) D.AliveCount++;
            else if (E.EventType == TEXT("Conflict")) D.ConflictCount++;
            ToMove++;
        }
        else
        {
            break;
        }
    }

    if (ToMove > 0)
    {
        ColdDigests.Add(D);
        WarmEvents.RemoveAt(0, ToMove, true);
    }
}

void UWorldHistoryLifecycleSubsystem::EvictAndDigest(int32 KeepFromIndex, int32 KeepToIndex)
{
    if (KeepFromIndex > KeepToIndex || KeepFromIndex < 0) return;
    // 预留：后续按时间回放窗口做更精准剪枝
}

int32 UWorldHistoryLifecycleSubsystem::GetHotEventCount() const
{
    return HotEvents.Num();
}

int32 UWorldHistoryLifecycleSubsystem::GetWarmEventCount() const
{
    return WarmEvents.Num();
}

TArray<FEventDigest> UWorldHistoryLifecycleSubsystem::GetDigestsInRange(FDateTime From, FDateTime To) const
{
    TArray<FEventDigest> Out;
    for (const FEventDigest& D : ColdDigests)
    {
        if (D.Day >= From && D.Day <= To)
        {
            Out.Add(D);
        }
    }
    return Out;
}
```

---

## 3) 改造 `TruthLedgerSubsystem`：接入生命周期

在 `TruthLedgerSubsystem.h` 里做最小字段调整：

```cpp
// 增加生命周期回传接口（可选）
UFUNCTION(BlueprintCallable, Category = "WorldSim|Truth")
void FeedLifecycle(TArray<FWorldEvent>& HotOut, TArray<FWorldEvent>& WarmOut, TArray<FEventDigest>& ColdOut) const;
```

在 `TruthLedgerSubsystem.cpp` 里，把 `Events` 也当“热+温入口”，并可同步给生命周期管理器（演示版）。

---

### 示例修改（完整可直接拼接）

```cpp
#include "WorldHistoryLifecycleSubsystem.h"
#include "Engine/World.h"

void UTruthLedgerSubsystem::RecordEvent(const FWorldEvent& Event)
{
    FWorldEvent Copy = Event;
    if (!Copy.EventId.IsValid())
    {
        Copy.EventId = FGuid::NewGuid();
    }
    Events.Add(Copy);

    if (UWorld* World = GetWorld())
    {
        if (auto* Life = World->GetSubsystem<UWorldHistoryLifecycleSubsystem>())
        {
            Life->ArchiveTick(Copy.EventTime, Events.Num());
        }
    }
}

void UTruthLedgerSubsystem::FeedLifecycle(TArray<FWorldEvent>& HotOut, TArray<FWorldEvent>& WarmOut, TArray<FEventDigest>& ColdOut) const
{
    const int32 N = Events.Num();
    if (N <= 50000)
    {
        HotOut = Events;
        return;
    }

    const int32 Split = N / 2;
    for (int32 i = 0; i < N; ++i)
    {
        if (i < Split)
        {
            HotOut.Add(Events[i]);
        }
        else
        {
            WarmOut.Add(Events[i]);
        }
    }

    if (WarmOut.Num() > 0)
    {
        FEventDigest D;
        D.Day = WarmOut[0].EventTime;
        D.TotalEventCount = WarmOut.Num();
        ColdOut.Add(D);
    }
}
```

---

## 4) 让蓝图看到冷热状态（`WorldSimBlueprintFunctionLibrary`）

继续在 `WorldSimBlueprintFunctionLibrary.h` 加：

```cpp
UFUNCTION(BlueprintPure, Category = "WorldSim|Lifecycle")
static FLifecycleConfig BS_GetLifecycleConfig(const UObject* WorldContextObject);

UFUNCTION(BlueprintCallable, Category = "WorldSim|Lifecycle", meta = (WorldContext = "WorldContextObject"))
static TArray<FEventDigest> BS_GetColdDigests(const UObject* WorldContextObject, FDateTime From, FDateTime To);

UFUNCTION(BlueprintCallable, Category = "WorldSim|Lifecycle", meta = (WorldContext = "WorldContextObject"))
static int32 BS_GetHotEventCount(const UObject* WorldContextObject);
```

在 `WorldSimBlueprintFunctionLibrary.cpp` 追加：

```cpp
#include "WorldHistoryLifecycleSubsystem.h"

FLifecycleConfig UWorldSimBlueprintFunctionLibrary::BS_GetLifecycleConfig(const UObject* WorldContextObject)
{
    UWorld* World = GEngine ? GEngine->GetWorldFromContextObjectChecked(WorldContextObject) : nullptr;
    if (!World) return FLifecycleConfig();

    if (auto* Life = World->GetSubsystem<UWorldHistoryLifecycleSubsystem>())
    {
        return Life->GetConfig();
    }
    return FLifecycleConfig();
}

TArray<FEventDigest> UWorldSimBlueprintFunctionLibrary::BS_GetColdDigests(const UObject* WorldContextObject, FDateTime From, FDateTime To)
{
    UWorld* World = GEngine ? GEngine->GetWorldFromContextObjectChecked(WorldContextObject) : nullptr;
    if (!World) return {};

    if (auto* Life = World->GetSubsystem<UWorldHistoryLifecycleSubsystem>())
    {
        return Life->GetDigestsInRange(From, To);
    }
    return {};
}

int32 UWorldSimBlueprintFunctionLibrary::BS_GetHotEventCount(const UObject* WorldContextObject)
{
    UWorld* World = GEngine ? GEngine->GetWorldFromContextObjectChecked(WorldContextObject) : nullptr;
    if (!World) return 0;

    if (auto* Life = World->GetSubsystem<UWorldHistoryLifecycleSubsystem>())
    {
        return Life->GetHotEventCount();
    }
    return 0;
}
```

---

## 5) 阶段7 验收清单

1. 连续写入 N 条事件后，`BS_GetHotEventCount` 不会无限增长（触发归档或降采样）。
2. `ArchiveTick` 可将历史事件按时间窗转入温层（日志量被控）。
3. 当温层量超阈值，最早天数据能进入 `ColdDigests`。
4. 蓝图能读到冷数据摘要（至少返回非空）。
5. 全流程仍可回放关键行为：`CommitAccepted / Departed / Arrived` 仍保留至少在热或温区，能用于最近逻辑判断。
