# 阶段6：行星内交通与短程旅行（可贴代码包）

目标：先做“城市/节点图+边权+容量+票务流程”，再把结果映射到 `Commitment + Presence + TruthLedger`，形成可回放、可一致性的短途旅行链路。

文件清单：
- `Source/WorldSimDemo/Public/WorldTransportTypes.h`
- `Source/WorldSimDemo/Public/WorldTransportGraphSubsystem.h/.cpp`
- `Source/WorldSimDemo/Public/TravelServiceSubsystem.h/.cpp`
- `WorldSimBlueprintFunctionLibrary` 旅行入口

---

## 1) `Source/WorldSimDemo/Public/WorldTransportTypes.h`

```cpp
#pragma once

#include "CoreMinimal.h"
#include "WorldTransportTypes.generated.h"

UENUM(BlueprintType)
enum class ETransportMode : uint8
{
    Bus UMETA(DisplayName = "Bus"),
    Rail UMETA(DisplayName = "Rail"),
    Air UMETA(DisplayName = "Air"),
    Sea UMETA(DisplayName = "Sea")
};

UENUM(BlueprintType)
enum class ETravelState : uint8
{
    Idle UMETA(DisplayName = "Idle"),
    Booked UMETA(DisplayName = "Booked"),
    Enroute UMETA(DisplayName = "Enroute"),
    Arrived UMETA(DisplayName = "Arrived"),
    Cancelled UMETA(DisplayName = "Cancelled"),
    Failed UMETA(DisplayName = "Failed")
};

USTRUCT(BlueprintType)
struct FTransportNode
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    int32 NodeId = INDEX_NONE;

    UPROPERTY(BlueprintReadWrite)
    FString NodeName;

    UPROPERTY(BlueprintReadWrite)
    int32 RegionId = INDEX_NONE;

    UPROPERTY(BlueprintReadWrite)
    FVector Location = FVector::ZeroVector;
};

USTRUCT(BlueprintType)
struct FTransportEdge
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    int32 EdgeId = INDEX_NONE;

    UPROPERTY(BlueprintReadWrite)
    int32 FromNodeId = INDEX_NONE;

    UPROPERTY(BlueprintReadWrite)
    int32 ToNodeId = INDEX_NONE;

    UPROPERTY(BlueprintReadWrite)
    float DurationHours = 1.0f;

    UPROPERTY(BlueprintReadWrite)
    float Cost = 10.0f;

    UPROPERTY(BlueprintReadWrite)
    int32 Capacity = 200;

    UPROPERTY(BlueprintReadWrite)
    ETransportMode Mode = ETransportMode::Bus;
};

USTRUCT(BlueprintType)
struct FTravelTicket
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FGuid TicketId;

    UPROPERTY(BlueprintReadWrite)
    FGuid PersonId;

    UPROPERTY(BlueprintReadWrite)
    int32 FromNodeId = INDEX_NONE;

    UPROPERTY(BlueprintReadWrite)
    int32 ToNodeId = INDEX_NONE;

    UPROPERTY(BlueprintReadWrite)
    int32 EdgeId = INDEX_NONE;

    UPROPERTY(BlueprintReadWrite)
    FDateTime BookAt = FDateTime::UtcNow();

    UPROPERTY(BlueprintReadWrite)
    FDateTime DepartAt = FDateTime::MinValue();

    UPROPERTY(BlueprintReadWrite)
    FDateTime ArriveAt = FDateTime::MinValue();

    UPROPERTY(BlueprintReadWrite)
    ETravelState State = ETravelState::Idle;

    UPROPERTY(BlueprintReadWrite)
    int32 SeatsReserved = 0;

    UPROPERTY(BlueprintReadWrite)
    float Paid = 0.0f;

    UPROPERTY(BlueprintReadWrite)
    FGuid CommitmentId;
};
```

---

## 2) `WorldTransportGraphSubsystem`

### `Source/WorldSimDemo/Public/WorldTransportGraphSubsystem.h`

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WorldTransportTypes.h"
#include "WorldTransportGraphSubsystem.generated.h"

USTRUCT(BlueprintType)
struct FRouteRequest
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    int32 FromNodeId = INDEX_NONE;

    UPROPERTY(BlueprintReadWrite)
    int32 ToNodeId = INDEX_NONE;
};

UCLASS()
class WORLDSIMDEMO_API UWorldTransportGraphSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category = "WorldSim|Transport")
    void RegisterNode(const FTransportNode& Node);

    UFUNCTION(BlueprintCallable, Category = "WorldSim|Transport")
    void RegisterEdge(const FTransportEdge& Edge);

    UFUNCTION(BlueprintPure, Category = "WorldSim|Transport")
    bool CanTravelBetween(int32 FromNodeId, int32 ToNodeId, FTransportEdge& OutBestEdge) const;

    UFUNCTION(BlueprintPure, Category = "WorldSim|Transport")
    TArray<int32> GetNodeIds() const;

    UFUNCTION(BlueprintPure, Category = "WorldSim|Transport")
    TArray<FTransportEdge> GetOutgoingEdges(int32 NodeId) const;

private:
    UPROPERTY()
    TMap<int32, FTransportNode> Nodes;

    UPROPERTY()
    TArray<FTransportEdge> Edges;
};
```

### `Source/WorldSimDemo/Private/WorldTransportGraphSubsystem.cpp`

```cpp
#include "WorldTransportGraphSubsystem.h"

void UWorldTransportGraphSubsystem::RegisterNode(const FTransportNode& Node)
{
    Nodes.FindOrAdd(Node.NodeId) = Node;
}

void UWorldTransportGraphSubsystem::RegisterEdge(const FTransportEdge& Edge)
{
    FTransportEdge NewEdge = Edge;
    if (NewEdge.EdgeId == INDEX_NONE)
    {
        NewEdge.EdgeId = Edges.Num() + 1;
    }
    Edges.Add(NewEdge);
}

bool UWorldTransportGraphSubsystem::CanTravelBetween(int32 FromNodeId, int32 ToNodeId, FTransportEdge& OutBestEdge) const
{
    float BestCost = TNumericLimits<float>::Max();
    bool bFound = false;

    for (const FTransportEdge& E : Edges)
    {
        if (E.FromNodeId == FromNodeId && E.ToNodeId == ToNodeId && E.Capacity > 0)
        {
            if (E.Cost < BestCost)
            {
                BestCost = E.Cost;
                OutBestEdge = E;
                bFound = true;
            }
        }
    }
    return bFound;
}

TArray<int32> UWorldTransportGraphSubsystem::GetNodeIds() const
{
    TArray<int32> Out;
    Nodes.GetKeys(Out);
    return Out;
}

TArray<FTransportEdge> UWorldTransportGraphSubsystem::GetOutgoingEdges(int32 NodeId) const
{
    TArray<FTransportEdge> Out;
    for (const FTransportEdge& E : Edges)
    {
        if (E.FromNodeId == NodeId)
        {
            Out.Add(E);
        }
    }
    return Out;
}
```

---

## 3) `TravelServiceSubsystem`（买票/出发/在途/到达）

### `Source/WorldSimDemo/Public/TravelServiceSubsystem.h`

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "WorldTransportTypes.h"
#include "WorldSimCoreTypes.h"
#include "TravelServiceSubsystem.generated.h"

UCLASS()
class WORLDSIMDEMO_API UTravelServiceSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "WorldSim|Travel")
    FGuid BuyTicket(FGuid PersonId, int32 FromNodeId, int32 ToNodeId, FDateTime DepartureTime, bool bReturn, float MaxBudget = 100000.0f);

    UFUNCTION(BlueprintCallable, Category = "WorldSim|Travel")
    bool ConfirmDeparture(FGuid TicketId);

    UFUNCTION(BlueprintCallable, Category = "WorldSim|Travel")
    void TickTravel(FDateTime Now);

    UFUNCTION(BlueprintCallable, Category = "WorldSim|Travel")
    FTravelTicket GetTicket(FGuid TicketId) const;

    UFUNCTION(BlueprintCallable, Category = "WorldSim|Travel")
    TArray<FGuid> GetPersonTickets(FGuid PersonId) const;

    UFUNCTION(BlueprintCallable, Category = "WorldSim|Travel")
    bool CancelTicket(FGuid TicketId, const FString& Reason = TEXT("Cancelled"));

private:
    FGuid CreateCommitmentForTicket(const FTravelTicket& Ticket, int32 TargetRegionId, const FString& TypeName);

    UPROPERTY()
    TMap<FGuid, FTravelTicket> Tickets;

    UPROPERTY()
    TMap<FGuid, TArray<FGuid>> TicketsByPerson;

    UPROPERTY()
    TMap<int32, int32> EdgeOccupancy;
};
```

### `Source/WorldSimDemo/Private/TravelServiceSubsystem.cpp`

```cpp
#include "TravelServiceSubsystem.h"
#include "WorldTransportGraphSubsystem.h"
#include "PresenceSubsystem.h"
#include "CommitmentSubsystem.h"
#include "TruthLedgerSubsystem.h"
#include "RegionSnapshotSubsystem.h"
#include "WorldTimeSubsystem.h"
#include "Kismet/KismetMathLibrary.h"

FGuid UTravelServiceSubsystem::BuyTicket(FGuid PersonId, int32 FromNodeId, int32 ToNodeId, FDateTime DepartureTime, bool bReturn, float MaxBudget)
{
    if (!PersonId.IsValid() || FromNodeId == ToNodeId)
    {
        return FGuid();
    }

    UWorld* World = GetWorld();
    if (!World) return FGuid();
    auto* Graph = World->GetSubsystem<UWorldTransportGraphSubsystem>();
    if (!Graph) return FGuid();

    FTransportEdge Edge;
    if (!Graph->CanTravelBetween(FromNodeId, ToNodeId, Edge))
    {
        return FGuid();
    }

    if (Edge.Cost > MaxBudget)
    {
        return FGuid();
    }

    if (EdgeOccupancy.FindRef(Edge.EdgeId) >= Edge.Capacity)
    {
        return FGuid();
    }

    FGuid TicketId = FGuid::NewGuid();
    FTravelTicket Ticket;
    Ticket.TicketId = TicketId;
    Ticket.PersonId = PersonId;
    Ticket.FromNodeId = FromNodeId;
    Ticket.ToNodeId = ToNodeId;
    Ticket.EdgeId = Edge.EdgeId;
    Ticket.BookAt = FDateTime::UtcNow();
    Ticket.DepartAt = DepartureTime;
    Ticket.ArriveAt = DepartureTime + FTimespan::FromHours(Edge.DurationHours);
    Ticket.State = ETravelState::Booked;
    Ticket.SeatsReserved = 1;
    Ticket.Paid = Edge.Cost;

    Tickets.Add(TicketId, Ticket);
    TicketsByPerson.FindOrAdd(PersonId).Add(TicketId);
    EdgeOccupancy.FindOrAdd(Edge.EdgeId)++;
    return TicketId;
}

bool UTravelServiceSubsystem::ConfirmDeparture(FGuid TicketId)
{
    FTravelTicket* Ticket = Tickets.Find(TicketId);
    if (!Ticket || Ticket->State != ETravelState::Booked) return false;

    UWorld* World = GetWorld();
    if (!World) return false;

    auto* PresenceSub = World->GetSubsystem<UPresenceSubsystem>();
    auto* ComSub = World->GetSubsystem<UCommitmentSubsystem>();
    auto* TruthSub = World->GetSubsystem<UTruthLedgerSubsystem>();
    auto* GraphSub = World->GetSubsystem<UWorldTransportGraphSubsystem>();
    if (!PresenceSub || !ComSub || !TruthSub || !GraphSub) return false;

    // 当前时间节点->对应边
    FTransportEdge Edge;
    if (!GraphSub->CanTravelBetween(Ticket->FromNodeId, Ticket->ToNodeId, Edge))
    {
        return false;
    }

    // 写入承诺
    FCommitmentRecord C;
    C.PersonId = Ticket->PersonId;
    C.Type = ECommitmentType::Transit;
    C.EarliestStart = Ticket->DepartAt;
    C.LatestStart = Ticket->DepartAt + FTimespan::FromMinutes(5);
    C.ExpectedEnd = Ticket->ArriveAt;
    C.FromLocationId = Ticket->FromNodeId;
    C.ToLocationId = Ticket->ToNodeId;
    C.bHardCommit = true;
    C.bCancelable = true;

    const FGuid CommitmentId = ComSub->CreateCommitment(C);
    Ticket->CommitmentId = CommitmentId;

    FPresenceInterval P;
    P.PersonId = Ticket->PersonId;
    P.StartTime = Ticket->DepartAt;
    P.EndTime = Ticket->ArriveAt;
    P.RegionId = Ticket->ToNodeId;
    P.ActivityTag = EPersonActivityState::InTransit;
    P.CommitmentId = CommitmentId;

    FString Reject;
    if (!PresenceSub->TryReservePresence(P, &Reject))
    {
        Ticket->State = ETravelState::Failed;
        ComSub->UpdateCommitmentState(CommitmentId, ECommitmentState::Failed, Reject);
        return false;
    }

    Ticket->State = ETravelState::Enroute;
    ComSub->UpdateCommitmentState(CommitmentId, ECommitmentState::Executing, TEXT("departed"));

    FWorldEvent Event;
    Event.EventId = FGuid::NewGuid();
    Event.SourceCommitmentId = CommitmentId;
    Event.PersonId = Ticket->PersonId;
    Event.EventType = TEXT("Departed");
    Event.EventTime = FDateTime::UtcNow();
    Event.LocationId = Ticket->FromNodeId;
    Event.PayloadJson = TEXT("{\"To\": ") + FString::FromInt(Ticket->ToNodeId) + TEXT("}");
    TruthSub->RecordEvent(Event);

    return true;
}

void UTravelServiceSubsystem::TickTravel(FDateTime Now)
{
    UWorld* World = GetWorld();
    auto* TruthSub = World ? World->GetSubsystem<UTruthLedgerSubsystem>() : nullptr;
    auto* PresenceSub = World ? World->GetSubsystem<UPresenceSubsystem>() : nullptr;
    auto* ComSub = World ? World->GetSubsystem<UCommitmentSubsystem>() : nullptr;

    for (auto& Pair : Tickets)
    {
        FTravelTicket& Ticket = Pair.Value;
        if (Ticket.State != ETravelState::Enroute)
        {
            continue;
        }

        if (Now >= Ticket.ArriveAt)
        {
            Ticket.State = ETravelState::Arrived;

            if (PresenceSub)
            {
                PresenceSub->ReleasePresence(Ticket.PersonId, Ticket.ArriveAt);
                FPresenceInterval ArriveP;
                ArriveP.PersonId = Ticket.PersonId;
                ArriveP.StartTime = Ticket.ArriveAt;
                ArriveP.EndTime = Ticket.ArriveAt + FTimespan::FromDays(1);
                ArriveP.RegionId = Ticket.ToNodeId;
                ArriveP.ActivityTag = EPersonActivityState::Idle;
                ArriveP.CommitmentId = Ticket.CommitmentId;
                FString R;
                PresenceSub->TryReservePresence(ArriveP, &R);
            }

            if (ComSub)
            {
                ComSub->UpdateCommitmentState(Ticket.CommitmentId, ECommitmentState::Completed, TEXT("arrived"));
            }

            if (TruthSub)
            {
                FWorldEvent E;
                E.EventId = FGuid::NewGuid();
                E.SourceCommitmentId = Ticket.CommitmentId;
                E.PersonId = Ticket.PersonId;
                E.EventType = TEXT("Arrived");
                E.EventTime = Now;
                E.LocationId = Ticket.ToNodeId;
                E.PayloadJson = TEXT("{\"travel_end\": true}");
                TruthSub->RecordEvent(E);
            }

            if (auto* Region = World->GetSubsystem<URegionSnapshotSubsystem>())
            {
                Region->ApplyDelta(Ticket.ToNodeId, +1, 0.0f, 0.0f);
                Region->ApplyTransportDelta(Ticket.ToNodeId, +0.08f);
            }
        }
    }
}

FTravelTicket UTravelServiceSubsystem::GetTicket(FGuid TicketId) const
{
    if (const FTravelTicket* Found = Tickets.Find(TicketId))
    {
        return *Found;
    }
    return FTravelTicket();
}

TArray<FGuid> UTravelServiceSubsystem::GetPersonTickets(FGuid PersonId) const
{
    if (const TArray<FGuid>* Arr = TicketsByPerson.Find(PersonId))
    {
        return *Arr;
    }
    return {};
}

bool UTravelServiceSubsystem::CancelTicket(FGuid TicketId, const FString& Reason)
{
    FTravelTicket* T = Tickets.Find(TicketId);
    if (!T) return false;

    T->State = ETravelState::Cancelled;

    if (auto* ComSub = GetWorld() ? GetWorld()->GetSubsystem<UCommitmentSubsystem>() : nullptr)
    {
        ComSub->CancelCommitment(T->CommitmentId, Reason);
    }
    return true;
}

FGuid UTravelServiceSubsystem::CreateCommitmentForTicket(const FTravelTicket& Ticket, int32 TargetRegionId, const FString& TypeName)
{
    if (!GetWorld()) return FGuid();

    FCommitmentRecord C;
    C.PersonId = Ticket.PersonId;
    C.Type = ECommitmentType::Transit;
    C.EarliestStart = Ticket.DepartAt;
    C.LatestStart = Ticket.DepartAt + FTimespan::FromMinutes(2);
    C.ExpectedEnd = Ticket.ArriveAt;
    C.FromLocationId = Ticket.FromNodeId;
    C.ToLocationId = Ticket.ToNodeId;
    C.bHardCommit = true;

    return GetWorld()->GetSubsystem<UCommitmentSubsystem>()->CreateCommitment(C);
}
```

---

## 4) `WorldSimBlueprintFunctionLibrary` 旅行接口（追加）

在 `WorldSimBlueprintFunctionLibrary.h` 里加：

```cpp
UFUNCTION(BlueprintCallable, Category = "WorldSim|Travel", meta = (WorldContext = "WorldContextObject"))
static FGuid BS_BuyTicket(const UObject* WorldContextObject, FGuid PersonId, int32 FromNodeId, int32 ToNodeId, FDateTime DepartureTime, bool bReturn, float MaxBudget);

UFUNCTION(BlueprintCallable, Category = "WorldSim|Travel", meta = (WorldContext = "WorldContextObject"))
static bool BS_ConfirmDeparture(const UObject* WorldContextObject, FGuid TicketId);

UFUNCTION(BlueprintCallable, Category = "WorldSim|Travel", meta = (WorldContext = "WorldContextObject"))
static bool BS_TickTravel(const UObject* WorldContextObject);
```

```cpp
#include "TravelServiceSubsystem.h"
#include "WorldTimeSubsystem.h"

FGuid UWorldSimBlueprintFunctionLibrary::BS_BuyTicket(const UObject* WorldContextObject, FGuid PersonId, int32 FromNodeId, int32 ToNodeId, FDateTime DepartureTime, bool bReturn, float MaxBudget)
{
    if (!WorldContextObject) return FGuid();
    UGameInstance* GI = GEngine->GetWorldFromContextObjectChecked(WorldContextObject)->GetGameInstance();
    if (!GI) return FGuid();
    if (auto* S = GI->GetSubsystem<UTravelServiceSubsystem>())
    {
        return S->BuyTicket(PersonId, FromNodeId, ToNodeId, DepartureTime, bReturn, MaxBudget);
    }
    return FGuid();
}

bool UWorldSimBlueprintFunctionLibrary::BS_ConfirmDeparture(const UObject* WorldContextObject, FGuid TicketId)
{
    if (!WorldContextObject) return false;
    UGameInstance* GI = GEngine->GetWorldFromContextObjectChecked(WorldContextObject)->GetGameInstance();
    if (!GI) return false;
    if (auto* S = GI->GetSubsystem<UTravelServiceSubsystem>())
    {
        return S->ConfirmDeparture(TicketId);
    }
    return false;
}

bool UWorldSimBlueprintFunctionLibrary::BS_TickTravel(const UObject* WorldContextObject)
{
    if (!WorldContextObject) return false;
    UGameInstance* GI = GEngine->GetWorldFromContextObjectChecked(WorldContextObject)->GetGameInstance();
    if (!GI) return false;
    auto* TimeSub = GI->GetWorld()->GetSubsystem<UWorldTimeSubsystem>();
    if (!TimeSub) return false;

    if (auto* S = GI->GetSubsystem<UTravelServiceSubsystem>())
    {
        S->TickTravel(TimeSub->GetNow());
        return true;
    }
    return false;
}
```

---

## 5) 阶段6 快速验收

1. 注册2~3个交通节点和边。
2. `BS_BuyTicket` 买票成功返回 TicketId。
3. `BS_ConfirmDeparture` 成功后 `Ticket.State = Enroute`。
4. 时间推进到达点后调用 `BS_TickTravel`，见到 `Ticket.State = Arrived`，并有 `Arrived` 事件写入真相层。
5. 到达地与起点在区域快照里都能看到人口/运输负载变化。

有了这个版，阶段6“短程旅行”可进入可玩闭环测试。
