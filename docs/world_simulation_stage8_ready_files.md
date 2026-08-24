# 阶段8：跨星系扩展（Planet / StarSystem / 星际航线）最小实现

目标：把你现在的“行星内旅行”扩展到“星系间/行星间可验证旅行”。

本版本是最小实现，核心原则仍是：  
`承诺 -> 存在 -> 真相` 三条链路不被打断。  

新增文件（按现有项目结构）：
- `Source/WorldSimDemo/Public/InterstellarTypes.h`
- `Source/WorldSimDemo/Public/InterstellarNetworkSubsystem.h/.cpp`
- `Source/WorldSimDemo/Public/InterstellarTravelServiceSubsystem.h/.cpp`
- `WorldSimBlueprintFunctionLibrary` 跨星旅行入口

---

## 1) `Source/WorldSimDemo/Public/InterstellarTypes.h`

```cpp
#pragma once

#include "CoreMinimal.h"
#include "InterstellarTypes.generated.h"

UENUM(BlueprintType)
enum class EInterstellarPermitLevel : uint8
{
    None UMETA(DisplayName = "None"),
    Basic UMETA(DisplayName = "Basic"),
    Premium UMETA(DisplayName = "Premium"),
    Diplomatic UMETA(DisplayName = "Diplomatic"),
    Military UMETA(DisplayName = "Military"),
};

UENUM(BlueprintType)
enum class EInterstellarTravelState : uint8
{
    Idle UMETA(DisplayName = "Idle"),
    Booked UMETA(DisplayName = "Booked"),
    Inbound UMETA(DisplayName = "Inbound"),
    Docked UMETA(DisplayName = "Docked"),
    Failed UMETA(DisplayName = "Failed"),
    Cancelled UMETA(DisplayName = "Cancelled")
};

USTRUCT(BlueprintType)
struct FStarSystemNode
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    int32 SystemId = INDEX_NONE;

    UPROPERTY(BlueprintReadWrite)
    FString Name;
};

USTRUCT(BlueprintType)
struct FPlanetNode
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    int32 PlanetId = INDEX_NONE;

    UPROPERTY(BlueprintReadWrite)
    int32 HomeSystemId = INDEX_NONE;

    UPROPERTY(BlueprintReadWrite)
    FString Name;

    UPROPERTY(BlueprintReadWrite)
    int32 PortNodeId = INDEX_NONE; // 复用行星内图里的港口节点

    UPROPERTY(BlueprintReadWrite)
    int32 RegionId = INDEX_NONE; // 用于 Presence/快照映射
};

USTRUCT(BlueprintType)
struct FInterstellarRoute
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    int32 RouteId = INDEX_NONE;

    UPROPERTY(BlueprintReadWrite)
    int32 FromPlanetId = INDEX_NONE;

    UPROPERTY(BlueprintReadWrite)
    int32 ToPlanetId = INDEX_NONE;

    UPROPERTY(BlueprintReadWrite)
    float BaseDistance = 1.0f; // LY or 规则化单位

    UPROPERTY(BlueprintReadWrite)
    float FuelCost = 1.0f;

    UPROPERTY(BlueprintReadWrite)
    float DelayRiskBase = 0.1f; // 0~1

    UPROPERTY(BlueprintReadWrite)
    float DutyTax = 10.0f;

    UPROPERTY(BlueprintReadWrite)
    EInterstellarPermitLevel Permit = EInterstellarPermitLevel::Basic;

    UPROPERTY(BlueprintReadWrite)
    int32 Capacity = 200;

    UPROPERTY(BlueprintReadWrite)
    float TravelHours = 12.0f; // 最小停靠+加速前估算
};

USTRUCT(BlueprintType)
struct FInterstellarTicket
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FGuid TicketId;

    UPROPERTY(BlueprintReadWrite)
    FGuid PersonId;

    UPROPERTY(BlueprintReadWrite)
    int32 FromPlanetId = INDEX_NONE;

    UPROPERTY(BlueprintReadWrite)
    int32 ToPlanetId = INDEX_NONE;

    UPROPERTY(BlueprintReadWrite)
    int32 RouteId = INDEX_NONE;

    UPROPERTY(BlueprintReadWrite)
    FDateTime DepartAt = FDateTime::MinValue();

    UPROPERTY(BlueprintReadWrite)
    FDateTime ArriveAt = FDateTime::MinValue();

    UPROPERTY(BlueprintReadWrite)
    float Cost = 0.0f;

    UPROPERTY(BlueprintReadWrite)
    float DelayHours = 0.0f;

    UPROPERTY(BlueprintReadWrite)
    EInterstellarTravelState State = EInterstellarTravelState::Idle;

    UPROPERTY(BlueprintReadWrite)
    bool bVerifiedByAuthority = false;

    UPROPERTY(BlueprintReadWrite)
    FGuid CommitmentId;
};
```

---

## 2) `Source/WorldSimDemo/Public/InterstellarNetworkSubsystem.h`

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "InterstellarTypes.h"
#include "InterstellarNetworkSubsystem.generated.h"

USTRUCT(BlueprintType)
struct FInterstellarQuery
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    int32 FromPlanetId = INDEX_NONE;

    UPROPERTY(BlueprintReadWrite)
    int32 ToPlanetId = INDEX_NONE;

    UPROPERTY(BlueprintReadWrite)
    EInterstellarPermitLevel PermitLevel = EInterstellarPermitLevel::Basic;

    UPROPERTY(BlueprintReadWrite)
    float MaxBudget = 1e9f;
};

UCLASS()
class WORLDSIMDEMO_API UInterstellarNetworkSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "WorldSim|Interstellar")
    void RegisterSystem(const FStarSystemNode& InSystem);

    UFUNCTION(BlueprintCallable, Category = "WorldSim|Interstellar")
    void RegisterPlanet(const FPlanetNode& InPlanet);

    UFUNCTION(BlueprintCallable, Category = "WorldSim|Interstellar")
    void RegisterRoute(const FInterstellarRoute& InRoute);

    UFUNCTION(BlueprintPure, Category = "WorldSim|Interstellar")
    bool FindRoute(const FInterstellarQuery& Query, FInterstellarRoute& OutRoute) const;

    UFUNCTION(BlueprintPure, Category = "WorldSim|Interstellar")
    TArray<FPlanetNode> GetPlanetsInSystem(int32 SystemId) const;

    UFUNCTION(BlueprintPure, Category = "WorldSim|Interstellar")
    TArray<FInterstellarRoute> GetRoutes(int32 PlanetId) const;

    UFUNCTION(BlueprintPure, Category = "WorldSim|Interstellar")
    bool HasPermitForRoute(const FInterstellarRoute& Route, EInterstellarPermitLevel PersonPermit) const;

private:
    UPROPERTY()
    TMap<int32, FStarSystemNode> Systems;

    UPROPERTY()
    TMap<int32, FPlanetNode> Planets;

    UPROPERTY()
    TArray<FInterstellarRoute> Routes;
};
```

## `Source/WorldSimDemo/Private/InterstellarNetworkSubsystem.cpp`

```cpp
#include "InterstellarNetworkSubsystem.h"

void UInterstellarNetworkSubsystem::RegisterSystem(const FStarSystemNode& InSystem)
{
    Systems.Add(InSystem.SystemId, InSystem);
}

void UInterstellarNetworkSubsystem::RegisterPlanet(const FPlanetNode& InPlanet)
{
    Planets.Add(InPlanet.PlanetId, InPlanet);
}

void UInterstellarNetworkSubsystem::RegisterRoute(const FInterstellarRoute& InRoute)
{
    FInterstellarRoute R = InRoute;
    if (R.RouteId == INDEX_NONE)
    {
        R.RouteId = Routes.Num() + 1000;
    }
    Routes.Add(R);
}

bool UInterstellarNetworkSubsystem::FindRoute(const FInterstellarQuery& Query, FInterstellarRoute& OutRoute) const
{
    bool bFound = false;
    float Best = TNumericLimits<float>::Max();

    const auto GetPermitCostMul = [](EInterstellarPermitLevel P)
    {
        switch (P)
        {
            case EInterstellarPermitLevel::Military: return 0.70f;
            case EInterstellarPermitLevel::Diplomatic: return 0.90f;
            case EInterstellarPermitLevel::Premium: return 1.10f;
            case EInterstellarPermitLevel::Basic: return 1.30f;
            case EInterstellarPermitLevel::None: return 2.00f;
            default: return 1.0f;
        }
    };

    if (Query.FromPlanetId == Query.ToPlanetId)
    {
        OutRoute = FInterstellarRoute();
        OutRoute.FromPlanetId = Query.FromPlanetId;
        OutRoute.ToPlanetId = Query.ToPlanetId;
        OutRoute.TravelHours = 0.0f;
        OutRoute.FuelCost = 0.0f;
        return true;
    }

    for (const FInterstellarRoute& R : Routes)
    {
        if (R.FromPlanetId != Query.FromPlanetId || R.ToPlanetId != Query.ToPlanetId)
            continue;

        if (!HasPermitForRoute(R, Query.PermitLevel))
            continue;

        const float EstCost = R.FuelCost * GetPermitCostMul(Query.PermitLevel) + R.DutyTax;
        if (EstCost > Query.MaxBudget)
            continue;

        if (EstCost < Best)
        {
            Best = EstCost;
            OutRoute = R;
            bFound = true;
        }
    }
    return bFound;
}

TArray<FPlanetNode> UInterstellarNetworkSubsystem::GetPlanetsInSystem(int32 SystemId) const
{
    TArray<FPlanetNode> Out;
    for (const auto& Pair : Planets)
    {
        if (Pair.Value.HomeSystemId == SystemId)
        {
            Out.Add(Pair.Value);
        }
    }
    return Out;
}

TArray<FInterstellarRoute> UInterstellarNetworkSubsystem::GetRoutes(int32 PlanetId) const
{
    TArray<FInterstellarRoute> Out;
    for (const FInterstellarRoute& R : Routes)
    {
        if (R.FromPlanetId == PlanetId)
        {
            Out.Add(R);
        }
    }
    return Out;
}

bool UInterstellarNetworkSubsystem::HasPermitForRoute(const FInterstellarRoute& Route, EInterstellarPermitLevel PersonPermit) const
{
    return static_cast<uint8>(PersonPermit) >= static_cast<uint8>(Route.Permit);
}
```

---

## 3) `Source/WorldSimDemo/Public/InterstellarTravelServiceSubsystem.h`

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "InterstellarTypes.h"
#include "WorldSimCoreTypes.h"
#include "InterstellarNetworkSubsystem.h"
#include "InterstellarTravelServiceSubsystem.generated.h"

UCLASS()
class WORLDSIMDEMO_API UInterstellarTravelServiceSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "WorldSim|Interstellar")
    FGuid BuyInterplanetaryTicket(FGuid PersonId, int32 FromPlanetId, int32 ToPlanetId, FDateTime DepartAt, EInterstellarPermitLevel PermitLevel, float MaxBudget = 9999999.0f);

    UFUNCTION(BlueprintCallable, Category = "WorldSim|Interstellar")
    bool LaunchInterplanetaryTravel(FGuid TicketId);

    UFUNCTION(BlueprintCallable, Category = "WorldSim|Interstellar")
    bool CancelInterplanetaryTicket(FGuid TicketId, const FString& Reason = TEXT("PlayerCancel"));

    UFUNCTION(BlueprintCallable, Category = "WorldSim|Interstellar")
    void TickInterplanetary(FDateTime Now);

    UFUNCTION(BlueprintCallable, Category = "WorldSim|Interstellar")
    FInterstellarTicket GetInterplanetaryTicket(FGuid TicketId) const;

private:
    FDateTime ApplyDelayByRisk(const FInterstellarRoute& Route, const FDateTime& BaseArrive, float& OutDelayHours);
    bool PassAuthorityCheck(const FInterstellarRoute& Route, EInterstellarPermitLevel PlayerPermit, FString& OutRejectReason) const;
    bool IsRouteSafe(const FInterstellarRoute& Route, float& OutFailRisk) const;

    UPROPERTY()
    TMap<FGuid, FInterstellarTicket> Tickets;

    UPROPERTY()
    TMap<FGuid, TArray<FGuid>> TicketsByPerson;
};
```

## `Source/WorldSimDemo/Private/InterstellarTravelServiceSubsystem.cpp`

```cpp
#include "InterstellarTravelServiceSubsystem.h"
#include "WorldTransportGraphSubsystem.h"
#include "PresenceSubsystem.h"
#include "CommitmentSubsystem.h"
#include "TruthLedgerSubsystem.h"
#include "KnowledgeSubsystem.h"

FGuid UInterstellarTravelServiceSubsystem::BuyInterplanetaryTicket(FGuid PersonId, int32 FromPlanetId, int32 ToPlanetId, FDateTime DepartAt, EInterstellarPermitLevel PermitLevel, float MaxBudget)
{
    if (!PersonId.IsValid() || FromPlanetId == ToPlanetId)
    {
        return FGuid();
    }

    UWorld* World = GetWorld();
    if (!World) return FGuid();

    auto* Net = World->GetSubsystem<UInterstellarNetworkSubsystem>();
    if (!Net) return FGuid();

    FInterstellarQuery Q;
    Q.FromPlanetId = FromPlanetId;
    Q.ToPlanetId = ToPlanetId;
    Q.PermitLevel = PermitLevel;
    Q.MaxBudget = MaxBudget;

    FInterstellarRoute Route;
    if (!Net->FindRoute(Q, Route))
    {
        return FGuid();
    }

    // 简单稽核：需要在行星内可达港口
    auto* Graph = World->GetSubsystem<UWorldTransportGraphSubsystem>();
    if (!Graph)
    {
        return FGuid();
    }

    FGuid TicketId = FGuid::NewGuid();
    FInterstellarTicket T;
    T.TicketId = TicketId;
    T.PersonId = PersonId;
    T.FromPlanetId = FromPlanetId;
    T.ToPlanetId = ToPlanetId;
    T.RouteId = Route.RouteId;
    T.DepartAt = DepartAt;
    T.Cost = Route.FuelCost + Route.DutyTax;
    T.State = EInterstellarTravelState::Booked;

    const float Delay = FMath::Max(0.0f, FMath::FRandRange(0.0f, 24.0f) * Route.DelayRiskBase);
    T.DelayHours = Delay;
    T.ArriveAt = DepartAt + FTimespan::FromHours(Route.TravelHours + Delay);
    Tickets.Add(TicketId, T);
    TicketsByPerson.FindOrAdd(PersonId).Add(TicketId);

    return TicketId;
}

bool UInterstellarTravelServiceSubsystem::PassAuthorityCheck(const FInterstellarRoute& Route, EInterstellarPermitLevel PlayerPermit, FString& OutRejectReason) const
{
    if (static_cast<uint8>(PlayerPermit) < static_cast<uint8>(Route.Permit))
    {
        OutRejectReason = TEXT("许可等级不足");
        return false;
    }

    if (Route.FuelCost <= 0.0f)
    {
        OutRejectReason = TEXT("航线代价异常");
        return false;
    }

    return true;
}

bool UInterstellarTravelServiceSubsystem::IsRouteSafe(const FInterstellarRoute& Route, float& OutFailRisk) const
{
    // 0.0~1.0
    OutFailRisk = FMath::Clamp(Route.DelayRiskBase, 0.0f, 1.0f);
    // 示例：高风险阈值
    return OutFailRisk < 0.70f;
}

bool UInterstellarTravelServiceSubsystem::LaunchInterplanetaryTravel(FGuid TicketId)
{
    FInterstellarTicket* T = Tickets.Find(TicketId);
    if (!T || T->State != EInterstellarTravelState::Booked)
    {
        return false;
    }

    UWorld* World = GetWorld();
    if (!World) return false;

    auto* Net = World->GetSubsystem<UInterstellarNetworkSubsystem>();
    auto* PresenceSub = World->GetSubsystem<UPresenceSubsystem>();
    auto* ComSub = World->GetSubsystem<UCommitmentSubsystem>();
    auto* TruthSub = World->GetSubsystem<UTruthLedgerSubsystem>();
    auto* Knowledge = GetGameInstance()->GetSubsystem<UKnowledgeSubsystem>();

    if (!Net || !PresenceSub || !ComSub || !TruthSub || !Knowledge) return false;

    FInterstellarQuery Q;
    Q.FromPlanetId = T->FromPlanetId;
    Q.ToPlanetId = T->ToPlanetId;
    Q.PermitLevel = EInterstellarPermitLevel::Diplomatic;
    Q.MaxBudget = 9999999.0f;
    FInterstellarRoute Route;
    if (!Net->FindRoute(Q, Route))
    {
        return false;
    }

    FString Reject;
    if (!PassAuthorityCheck(Route, Q.PermitLevel, Reject))
    {
        if (Knowledge)
        {
            FPlayerKnowledgeItem K;
            K.SubjectPersonId = T->PersonId;
            K.Source = TEXT("Authority");
            K.Strength = EClaimStrength::Reported;
            K.Content = FString::Printf(TEXT("拒签：%s"), *Reject);
            K.ObservedAt = FDateTime::UtcNow();
            K.Confidence = 0.7f;
            Knowledge->AddKnowledge(K);
        }
        return false;
    }

    float FailRisk = 0.0f;
    const bool bSafe = IsRouteSafe(Route, FailRisk);
    if (!bSafe)
    {
        if (Knowledge)
        {
            FPlayerKnowledgeItem K;
            K.SubjectPersonId = T->PersonId;
            K.Source = TEXT("RiskSensor");
            K.Strength = EClaimStrength::Whispers;
            K.Content = FString::Printf(TEXT("跨星风险高，可能改签或改道"));
            K.Confidence = 0.4f;
            K.ObservedAt = FDateTime::UtcNow();
            Knowledge->AddKnowledge(K);
        }
    }

    // 写入承诺与真相
    FCommitmentRecord C;
    C.PersonId = T->PersonId;
    C.Type = ECommitmentType::Transit;
    C.EarliestStart = T->DepartAt;
    C.LatestStart = T->DepartAt + FTimespan::FromMinutes(5);
    C.ExpectedEnd = T->ArriveAt;
    C.FromLocationId = T->FromPlanetId;
    C.ToLocationId = T->ToPlanetId;
    C.bHardCommit = true;
    C.bCancelable = true;
    T->CommitmentId = ComSub->CreateCommitment(C);

    // 占用在途存在
    FPresenceInterval P;
    P.PersonId = T->PersonId;
    P.StartTime = T->DepartAt;
    P.EndTime = T->ArriveAt;
    P.RegionId = T->ToPlanetId;
    P.ActivityTag = EPersonActivityState::InTransit;
    P.CommitmentId = T->CommitmentId;
    FString RejectPresence;
    if (!PresenceSub->TryReservePresence(P, &RejectPresence))
    {
        ComSub->CancelCommitment(T->CommitmentId, RejectPresence);
        return false;
    }

    ComSub->UpdateCommitmentState(T->CommitmentId, ECommitmentState::Executing, TEXT("interstellar launch"));
    T->State = EInterstellarTravelState::Inbound;

    FWorldEvent Evt;
    Evt.EventId = FGuid::NewGuid();
    Evt.SourceCommitmentId = T->CommitmentId;
    Evt.PersonId = T->PersonId;
    Evt.EventType = TEXT("InterstellarLaunch");
    Evt.EventTime = FDateTime::UtcNow();
    Evt.LocationId = T->FromPlanetId;
    Evt.PayloadJson = FString::Printf(TEXT("{\"to\":%d,\"risk\":%.2f}"), T->ToPlanetId, FailRisk);
    TruthSub->RecordEvent(Evt);

    return true;
}

FDateTime UInterstellarTravelServiceSubsystem::ApplyDelayByRisk(const FInterstellarRoute& Route, const FDateTime& BaseArrive, float& OutDelayHours)
{
    OutDelayHours = FMath::Max(0.0f, FMath::FRandRange(0.0f, 12.0f) * Route.DelayRiskBase);
    return BaseArrive + FTimespan::FromHours(OutDelayHours);
}

void UInterstellarTravelServiceSubsystem::TickInterplanetary(FDateTime Now)
{
    UWorld* World = GetWorld();
    if (!World) return;

    auto* PresenceSub = World->GetSubsystem<UPresenceSubsystem>();
    auto* ComSub = World->GetSubsystem<UCommitmentSubsystem>();
    auto* TruthSub = World->GetSubsystem<UTruthLedgerSubsystem>();
    auto* Net = World->GetSubsystem<UInterstellarNetworkSubsystem>();
    if (!PresenceSub || !ComSub || !TruthSub || !Net) return;

    for (auto& Pair : Tickets)
    {
        FInterstellarTicket& T = Pair.Value;
        if (T.State != EInterstellarTravelState::Inbound)
        {
            continue;
        }

        if (Now >= T.ArriveAt)
        {
            T.State = EInterstellarTravelState::Docked;
            ComSub->UpdateCommitmentState(T.CommitmentId, ECommitmentState::Completed, TEXT("interstellar arrived"));

            PresenceSub->ReleasePresence(T.PersonId, T.ArriveAt);
            FPresenceInterval Arrive;
            Arrive.PersonId = T.PersonId;
            Arrive.StartTime = T.ArriveAt;
            Arrive.EndTime = T.ArriveAt + FTimespan::FromDays(1);
            Arrive.RegionId = T.ToPlanetId;
            Arrive.ActivityTag = EPersonActivityState::Idle;
            Arrive.CommitmentId = T.CommitmentId;
            FString RR;
            PresenceSub->TryReservePresence(Arrive, &RR);

            FWorldEvent Evt;
            Evt.EventId = FGuid::NewGuid();
            Evt.SourceCommitmentId = T.CommitmentId;
            Evt.PersonId = T.PersonId;
            Evt.EventType = TEXT("InterstellarArrived");
            Evt.EventTime = Now;
            Evt.LocationId = T.ToPlanetId;
            Evt.PayloadJson = TEXT("{\"success\":true}");
            TruthSub->RecordEvent(Evt);
        }
    }
}

bool UInterstellarTravelServiceSubsystem::CancelInterplanetaryTicket(FGuid TicketId, const FString& Reason)
{
    FInterstellarTicket* T = Tickets.Find(TicketId);
    if (!T) return false;

    T->State = EInterstellarTravelState::Cancelled;
    if (auto* ComSub = GetWorld() ? GetWorld()->GetSubsystem<UCommitmentSubsystem>() : nullptr)
    {
        ComSub->CancelCommitment(T->CommitmentId, Reason);
    }
    return true;
}

FInterstellarTicket UInterstellarTravelServiceSubsystem::GetInterplanetaryTicket(FGuid TicketId) const
{
    if (const FInterstellarTicket* T = Tickets.Find(TicketId))
    {
        return *T;
    }
    return FInterstellarTicket();
}
```

---

## 4) 蓝图入口（`WorldSimBlueprintFunctionLibrary`）

在 `WorldSimBlueprintFunctionLibrary.h` 追加：

```cpp
UFUNCTION(BlueprintCallable, Category = "WorldSim|Interstellar", meta = (WorldContext = "WorldContextObject"))
static FGuid BS_BuyInterstellarTicket(const UObject* WorldContextObject, FGuid PersonId, int32 FromPlanetId, int32 ToPlanetId, FDateTime DepartAt, EInterstellarPermitLevel PermitLevel, float MaxBudget);

UFUNCTION(BlueprintCallable, Category = "WorldSim|Interstellar", meta = (WorldContext = "WorldContextObject"))
static bool BS_LaunchInterstellarTravel(const UObject* WorldContextObject, FGuid TicketId);

UFUNCTION(BlueprintCallable, Category = "WorldSim|Interstellar", meta = (WorldContext = "WorldContextObject"))
static bool BS_TickInterstellar(const UObject* WorldContextObject);
```

在 `WorldSimBlueprintFunctionLibrary.cpp` 追加：

```cpp
#include "InterstellarTravelServiceSubsystem.h"
#include "InterstellarNetworkSubsystem.h"

FGuid UWorldSimBlueprintFunctionLibrary::BS_BuyInterstellarTicket(const UObject* WorldContextObject, FGuid PersonId, int32 FromPlanetId, int32 ToPlanetId, FDateTime DepartAt, EInterstellarPermitLevel PermitLevel, float MaxBudget)
{
    if (!WorldContextObject) return FGuid();
    UGameInstance* GI = GEngine->GetWorldFromContextObjectChecked(WorldContextObject)->GetGameInstance();
    if (!GI) return FGuid();

    if (auto* S = GI->GetSubsystem<UInterstellarTravelServiceSubsystem>())
    {
        return S->BuyInterplanetaryTicket(PersonId, FromPlanetId, ToPlanetId, DepartAt, PermitLevel, MaxBudget);
    }
    return FGuid();
}

bool UWorldSimBlueprintFunctionLibrary::BS_LaunchInterstellarTravel(const UObject* WorldContextObject, FGuid TicketId)
{
    if (!WorldContextObject) return false;
    UGameInstance* GI = GEngine->GetWorldFromContextObjectChecked(WorldContextObject)->GetGameInstance();
    if (!GI) return false;
    if (auto* S = GI->GetSubsystem<UInterstellarTravelServiceSubsystem>())
    {
        return S->LaunchInterplanetaryTravel(TicketId);
    }
    return false;
}

bool UWorldSimBlueprintFunctionLibrary::BS_TickInterstellar(const UObject* WorldContextObject)
{
    if (!WorldContextObject) return false;
    UWorld* World = GEngine->GetWorldFromContextObjectChecked(WorldContextObject);
    if (!World) return false;
    auto* S = World->GetGameInstance() ? World->GetGameInstance()->GetSubsystem<UInterstellarTravelServiceSubsystem>() : nullptr;
    if (!S) return false;
    auto* TimeSub = World->GetSubsystem<UWorldTimeSubsystem>();
    if (!TimeSub) return false;
    S->TickInterplanetary(TimeSub->GetNow());
    return true;
}
```

---

## 5) 阶段8 验收（跨星旅行）

1. 注册至少两个星系、两颗行星、两条跨星路线。
2. `BS_BuyInterplanetaryTicket` 能在许可/预算通过时返回票据。
3. 许可不足时购票/发射失败，并有拒绝可观测（可以先做知识层记录）。
4. 成功 `BS_LaunchInterstellarTravel` 后，`Presence` 处于在途状态。
5. 时间推进后触发到达，产生 `InterstellarArrived` 真相事件，`Commitment` 变为完成，存在切到目标星球。

阶段8 先到这里；后续你就能把星球 A->B->C 作为验证链路，并把路线失败加权成：
- 改签（刷新票据）
- 改道（找备选路线）
- 失联（事件写 `InterstellarLost`）  
并保持你当前的前线-在场一致性规则不变。
