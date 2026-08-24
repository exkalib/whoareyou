# 阶段6：行星内交通与短程旅行模板

本阶段目标：
- 把“城市/港口/岗位点”做成可规划的图
- 支持买票、出发、在途、到达、停留、返回
- 把结果回写到 `TruthLedger` 与 `RegionSnapshot`

---

## 1) 交通节点与边的类型（`WorldSimTypes.h`）

```cpp
#pragma once

#include "CoreMinimal.h"
#include "WorldSimTypes.h"

UENUM(BlueprintType)
enum class ENodeType : uint8
{
    City UMETA(DisplayName = "City"),
    Port UMETA(DisplayName = "Port"),
    Hub UMETA(DisplayName = "Hub"),
    Worksite UMETA(DisplayName = "Worksite")
};

USTRUCT(BlueprintType)
struct FTransportNode
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    int32 NodeId = INDEX_NONE;

    UPROPERTY(BlueprintReadWrite)
    FName NodeName = NAME_None;

    UPROPERTY(BlueprintReadWrite)
    ENodeType NodeType = ENodeType::City;

    UPROPERTY(BlueprintReadWrite)
    FVector Location = FVector::ZeroVector;

    UPROPERTY(BlueprintReadWrite)
    int32 RegionId = INDEX_NONE;

    UPROPERTY(BlueprintReadWrite)
    bool bActive = true;
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
    float BaseMinutes = 60.0f;

    UPROPERTY(BlueprintReadWrite)
    float BaseCost = 30.0f;

    UPROPERTY(BlueprintReadWrite)
    int32 Capacity = 64;

    UPROPERTY(BlueprintReadWrite)
    bool bBidirectional = true;

    UPROPERTY(BlueprintReadWrite)
    float CongestionPenalty = 1.0f;

    UPROPERTY(BlueprintReadWrite)
    float Risk = 0.05f;
};

USTRUCT(BlueprintType)
struct FTravelTicket
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FGuid TicketId;

    UPROPERTY(BlueprintReadOnly)
    FGuid HolderPersonId;

    UPROPERTY(BlueprintReadOnly)
    FDateTime PurchaseTime;

    UPROPERTY(BlueprintReadOnly)
    int32 FromNodeId = INDEX_NONE;

    UPROPERTY(BlueprintReadOnly)
    int32 ToNodeId = INDEX_NONE;

    UPROPERTY(BlueprintReadOnly)
    FDateTime DepartureTime;

    UPROPERTY(BlueprintReadOnly)
    FDateTime ArrivalTime;

    UPROPERTY(BlueprintReadOnly)
    float Cost = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    bool bValid = true;

    UPROPERTY(BlueprintReadOnly)
    FString ClassName;
};

USTRUCT(BlueprintType)
struct FTravelRequest
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FGuid RequestId;

    UPROPERTY(BlueprintReadWrite)
    FGuid PersonId;

    UPROPERTY(BlueprintReadWrite)
    int32 FromRegionId = INDEX_NONE;

    UPROPERTY(BlueprintReadWrite)
    int32 ToRegionId = INDEX_NONE;

    UPROPERTY(BlueprintReadWrite)
    FDateTime DesiredDeparture;

    UPROPERTY(BlueprintReadWrite)
    bool bIsLeisure = false;

    UPROPERTY(BlueprintReadWrite)
    int32 MaxTransferCount = 2;

    UPROPERTY(BlueprintReadWrite)
    float MaxSpend = 500.0f;

    UPROPERTY(BlueprintReadWrite)
    float MaxRisk = 1.0f;
};

USTRUCT(BlueprintType)
struct FTravelPath
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    TArray<int32> EdgeIds;

    UPROPERTY(BlueprintReadWrite)
    TArray<int32> NodeIds;

    UPROPERTY(BlueprintReadWrite)
    float TotalMinutes = 0.0f;

    UPROPERTY(BlueprintReadWrite)
    float TotalCost = 0.0f;

    UPROPERTY(BlueprintReadWrite)
    float MaxRisk = 0.0f;

    UPROPERTY(BlueprintReadWrite)
    float MinCapacityRatio = 1.0f;
};

USTRUCT(BlueprintType)
struct FTravelState
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
    FDateTime DepartureTime;

    UPROPERTY(BlueprintReadWrite)
    FDateTime ETA;

    UPROPERTY(BlueprintReadWrite)
    FDateTime ReturnAt;

    UPROPERTY(BlueprintReadWrite)
    int32 CurrentEdgeIndex = 0;

    UPROPERTY(BlueprintReadWrite)
    TArray<int32> PlannedEdgeIds;

    UPROPERTY(BlueprintReadWrite)
    bool bReturning = false;

    UPROPERTY(BlueprintReadWrite)
    bool bCompleted = false;
};
```

---

## 2) 交通图系统（`WorldTransportGraphSubsystem.h/.cpp`）

### `WorldTransportGraphSubsystem.h`

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "WorldSimTypes.h"
#include "WorldTransportGraphSubsystem.generated.h"

UCLASS()
class WORLDSIMDEMO_API UWorldTransportGraphSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="WorldSim|Transport")
    void RegisterNode(const FTransportNode& Node);

    UFUNCTION(BlueprintCallable, Category="WorldSim|Transport")
    void RegisterEdge(const FTransportEdge& Edge);

    UFUNCTION(BlueprintCallable, Category="WorldSim|Transport")
    TArray<FTransportNode> GetNodes() const;

    UFUNCTION(BlueprintCallable, Category="WorldSim|Transport")
    TArray<FTransportEdge> GetEdges() const;

    UFUNCTION(BlueprintCallable, Category="WorldSim|Transport")
    int32 GetNodeForRegion(int32 RegionId) const;

    UFUNCTION(BlueprintCallable, Category="WorldSim|Transport")
    TArray<int32> GetNeighbors(int32 NodeId) const;

    UFUNCTION(BlueprintCallable, Category="WorldSim|Transport")
    bool FindPath(int32 FromNodeId, int32 ToNodeId, int32 MaxHops, const FRegionSnapshot& FromRegion, const FRegionSnapshot& ToRegion, FTravelPath& OutPath) const;

    UFUNCTION(BlueprintCallable, Category="WorldSim|Transport")
    float EstimateEdgeLoadPenalty(int32 EdgeId, const FRegionSnapshot& RegionSnapshot) const;

private:
    float GetNodeArrivalModifier(int32 NodeId) const;

    UPROPERTY()
    TMap<int32, FTransportNode> Nodes;

    UPROPERTY()
    TMap<int32, FTransportEdge> Edges;

    UPROPERTY()
    TMap<int32, TArray<int32>> Outgoing;
};
```

### `WorldTransportGraphSubsystem.cpp`

```cpp
#include "WorldTransportGraphSubsystem.h"

void UWorldTransportGraphSubsystem::RegisterNode(const FTransportNode& Node)
{
    Nodes.Add(Node.NodeId, Node);
}

void UWorldTransportGraphSubsystem::RegisterEdge(const FTransportEdge& Edge)
{
    Edges.Add(Edge.EdgeId, Edge);
    Outgoing.FindOrAdd(Edge.FromNodeId).Add(Edge.EdgeId);

    if (Edge.bBidirectional)
    {
        FTransportEdge Rev = Edge;
        Rev.EdgeId = -Edge.EdgeId;
        Swap(Rev.FromNodeId, Rev.ToNodeId);
        Edges.Add(Rev.EdgeId, Rev);
        Outgoing.FindOrAdd(Rev.FromNodeId).Add(Rev.EdgeId);
    }
}

TArray<FTransportNode> UWorldTransportGraphSubsystem::GetNodes() const
{
    TArray<FTransportNode> Out;
    Nodes.GenerateValueArray(Out);
    return Out;
}

TArray<FTransportEdge> UWorldTransportGraphSubsystem::GetEdges() const
{
    TArray<FTransportEdge> Out;
    Edges.GenerateValueArray(Out);
    return Out;
}

int32 UWorldTransportGraphSubsystem::GetNodeForRegion(int32 RegionId) const
{
    for (const auto& Pair : Nodes)
    {
        if (Pair.Value.RegionId == RegionId)
        {
            return Pair.Key;
        }
    }
    return INDEX_NONE;
}

TArray<int32> UWorldTransportGraphSubsystem::GetNeighbors(int32 NodeId) const
{
    TArray<int32> Out;
    const TArray<int32>* Adj = Outgoing.Find(NodeId);
    if (!Adj) return Out;
    for (int32 EdgeId : *Adj)
    {
        if (const FTransportEdge* E = Edges.Find(EdgeId))
        {
            Out.Add(E->ToNodeId);
        }
    }
    return Out;
}

float UWorldTransportGraphSubsystem::EstimateEdgeLoadPenalty(int32 EdgeId, const FRegionSnapshot& RegionSnapshot) const
{
    const FTransportEdge* E = Edges.Find(EdgeId);
    if (!E) return 1.0f;

    const float congestion = 1.0f + RegionSnapshot.TransportLoad * E->CongestionPenalty * 0.5f;
    return FMath::Clamp(congestion, 0.7f, 3.0f);
}

bool UWorldTransportGraphSubsystem::FindPath(int32 FromNodeId, int32 ToNodeId, int32 MaxHops, const FRegionSnapshot& FromRegion, const FRegionSnapshot& ToRegion, FTravelPath& OutPath) const
{
    // 简化路径搜索：最多 MaxHops 的 DFS/贪心，不做严格最短路径。
    // 适合作为第一版，后续可替换为 Dijkstra/A*
    if (FromNodeId == ToNodeId)
    {
        OutPath.NodeIds = {FromNodeId, ToNodeId};
        OutPath.TotalMinutes = 0.f;
        return true;
    }

    const TArray<int32>* StartAdj = Outgoing.Find(FromNodeId);
    if (!StartAdj || StartAdj->Num() == 0) return false;

    int32 SelectedEdge = INDEX_NONE;
    for (int32 EdgeId : *StartAdj)
    {
        if (const FTransportEdge* E = Edges.Find(EdgeId))
        {
            if (SelectedEdge == INDEX_NONE || (E->BaseCost < Edges[SelectedEdge].BaseCost))
            {
                SelectedEdge = EdgeId;
            }
        }
    }

    if (SelectedEdge == INDEX_NONE) return false;

    const FTransportEdge* FirstEdge = Edges.Find(SelectedEdge);
    if (!FirstEdge) return false;

    const float penalty = EstimateEdgeLoadPenalty(SelectedEdge, FromRegion);
    const float time = FirstEdge->BaseMinutes * penalty;

    OutPath.EdgeIds = {SelectedEdge};
    OutPath.NodeIds = {FromNodeId, FirstEdge->ToNodeId};
    OutPath.TotalMinutes = time;
    OutPath.TotalCost = FirstEdge->BaseCost;
    OutPath.MaxRisk = FirstEdge->Risk;
    OutPath.MinCapacityRatio = 1.0f;

    if (FirstEdge->ToNodeId == ToNodeId)
    {
        return true;
    }

    if (MaxHops <= 1)
    {
        return false;
    }

    // 只做一段转移，后续你可扩展多跳
    const TArray<int32>* NextAdj = Outgoing.Find(FirstEdge->ToNodeId);
    if (!NextAdj || NextAdj->Num() == 0) return false;

    // 尝试一次转移直接到目标节点
    for (int32 EdgeId2 : *NextAdj)
    {
        if (const FTransportEdge* E2 = Edges.Find(EdgeId2))
        {
            if (E2->ToNodeId == ToNodeId)
            {
                const float penalty2 = EstimateEdgeLoadPenalty(E2->EdgeId, ToRegion);
                OutPath.EdgeIds.Add(E2->EdgeId);
                OutPath.NodeIds.Add(ToNodeId);
                OutPath.TotalMinutes += E2->BaseMinutes * penalty2;
                OutPath.TotalCost += E2->BaseCost;
                OutPath.MaxRisk = FMath::Max(OutPath.MaxRisk, E2->Risk);
                return true;
            }
        }
    }

    return OutPath.NodeIds.Contains(ToNodeId);
}
```

---

## 3) 旅行服务系统（`TravelServiceSubsystem.h/.cpp`）

### `TravelServiceSubsystem.h`

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "WorldSimTypes.h"
#include "TravelServiceSubsystem.generated.h"

UCLASS()
class WORLDSIMDEMO_API UTravelServiceSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="WorldSim|Travel")
    FTravelTicket BuyTicket(const FTravelRequest& Request, const FTravelPath& Path, FDateTime DepartAt);

    UFUNCTION(BlueprintCallable, Category="WorldSim|Travel")
    bool CancelTicket(FGuid TicketId);

    UFUNCTION(BlueprintCallable, Category="WorldSim|Travel")
    bool IsTicketValid(FGuid TicketId) const;

    UFUNCTION(BlueprintCallable, Category="WorldSim|Travel")
    TArray<FTravelTicket> GetPersonTickets(FGuid PersonId) const;

    UFUNCTION(BlueprintCallable, Category="WorldSim|Travel")
    bool BindTravelAsCommitment(const FTravelTicket& Ticket, FCommitmentEvent& OutCommitment);

    UFUNCTION(BlueprintCallable, Category="WorldSim|Travel")
    bool CanReturnTomorrow(const FTravelTicket& Ticket, const FDateTime& Now) const;

private:
    UPROPERTY()
    TMap<FGuid, FTravelTicket> Tickets;

    UPROPERTY()
    TMap<FGuid, FTravelState> ActiveTravels;
};
```

### `TravelServiceSubsystem.cpp`

```cpp
#include "TravelServiceSubsystem.h"

FTravelTicket UTravelServiceSubsystem::BuyTicket(const FTravelRequest& Request, const FTravelPath& Path, FDateTime DepartAt)
{
    FTravelTicket T;
    T.TicketId = FGuid::NewGuid();
    T.HolderPersonId = Request.PersonId;
    T.PurchaseTime = FDateTime::UtcNow();
    T.FromNodeId = Path.NodeIds.Num() > 0 ? Path.NodeIds[0] : INDEX_NONE;
    T.ToNodeId = Path.NodeIds.Num() > 0 ? Path.NodeIds.Last() : INDEX_NONE;
    T.DepartureTime = DepartAt;
    T.ArrivalTime = DepartAt + FTimespan::FromMinutes(Path.TotalMinutes);
    T.Cost = Path.TotalCost;
    T.bValid = true;
    T.ClassName = Request.bIsLeisure ? TEXT("Tourism") : TEXT("Regular");

    Tickets.Add(T.TicketId, T);
    return T;
}

bool UTravelServiceSubsystem::CancelTicket(FGuid TicketId)
{
    if (!Tickets.Contains(TicketId))
    {
        return false;
    }
    FTravelTicket& T = Tickets[TicketId];
    T.bValid = false;
    return true;
}

bool UTravelServiceSubsystem::IsTicketValid(FGuid TicketId) const
{
    if (const FTravelTicket* T = Tickets.Find(TicketId))
    {
        return T->bValid;
    }
    return false;
}

TArray<FTravelTicket> UTravelServiceSubsystem::GetPersonTickets(FGuid PersonId) const
{
    TArray<FTravelTicket> Out;
    for (const auto& Pair : Tickets)
    {
        if (Pair.Value.HolderPersonId == PersonId)
        {
            Out.Add(Pair.Value);
        }
    }
    return Out;
}

bool UTravelServiceSubsystem::BindTravelAsCommitment(const FTravelTicket& Ticket, FCommitmentEvent& OutCommitment)
{
    if (!Ticket.bValid)
    {
        return false;
    }

    OutCommitment = FCommitmentEvent();
    OutCommitment.CommitmentId = FGuid::NewGuid();
    OutCommitment.PersonId = Ticket.HolderPersonId;
    OutCommitment.Type = ECommitmentType::Tourism;
    OutCommitment.EarliestStart = Ticket.DepartureTime;
    OutCommitment.LatestStart = Ticket.DepartureTime + FTimespan::FromMinutes(15);
    OutCommitment.ExpectedEnd = Ticket.ArrivalTime + FTimespan::FromMinutes(30);
    OutCommitment.FromLocationId = Ticket.FromNodeId;
    OutCommitment.ToLocationId = Ticket.ToNodeId;
    OutCommitment.bHardCommit = true;
    OutCommitment.CostBudget = Ticket.Cost;
    OutCommitment.bCancelable = true;

    // 记录 TicketId 关联到承诺可放在 JsonPayload 或单独映射表
    return true;
}

bool UTravelServiceSubsystem::CanReturnTomorrow(const FTravelTicket& Ticket, const FDateTime& Now) const
{
    return Now + FTimespan::FromDays(1) < Ticket.ArrivalTime;
}
```

---

## 4) 旅行状态机（接在 `WorldSimSchedulerSubsystem`）

### 扩展 `FCommitmentEvent`
- 在 `Type` 中加入 `ECommitmentType::Transit`（或 `Tourism`）
- `RouteId` 里可放 TicketId 的低位哈希（`GetTypeHash(TicketId)`）

### 在调度器里处理“在途/停留/返回”

```cpp
// 世界真相层建议新增几种 EventType：
// "Boarded", "InTransit", "Arrived", "Stay", "ReturnBoarded", "Returned"

void UWorldSimSchedulerSubsystem::UpdateTransit(FGuid CommitmentId, const FCommitmentEvent& C, const FDateTime& Now)
{
    // 1) 出发
    if (Now >= C.EarliestStart)
    {
        EmitEvent(TimeSub, TruthSub, CommitmentId, C.PersonId, TEXT("Boarded"), C.FromLocationId, TEXT("{ \"phase\": \"departed\" }"));
        EmitEvent(TimeSub, TruthSub, CommitmentId, C.PersonId, TEXT("InTransit"), C.ToLocationId, TEXT("{ \"phase\": \"route\" }"));

        // 给 RegionSnapshot 一个短暂“交通流量上升”写入
        RegionSub->ApplyMicroDelta(C.FromLocationId, 0, +2);
    }

    // 2) 到达
    if (Now >= C.ExpectedEnd)
    {
        EmitEvent(TimeSub, TruthSub, CommitmentId, C.PersonId, TEXT("Arrived"), C.ToLocationId, TEXT("{ \"phase\": \"arrived\" }"));

        // 在地停留 2小时
        FCommitmentEvent Return;
        Return.CommitmentId = FGuid::NewGuid();
        Return.PersonId = C.PersonId;
        Return.Type = ECommitmentType::Transit;
        Return.FromLocationId = C.ToLocationId;
        Return.ToLocationId = C.FromLocationId;
        Return.EarliestStart = C.ExpectedEnd;
        Return.LatestStart = C.ExpectedEnd + FTimespan::FromHours(1);
        Return.ExpectedEnd = C.ExpectedEnd + FTimespan::FromHours(1);
        Return.bHardCommit = true;

        CommitmentSub->CreateCommitment(Return);
        EmitEvent(TimeSub, TruthSub, CommitmentId, C.PersonId, TEXT("Stay"), C.ToLocationId, TEXT("{ \"phase\": \"leisure\" }"));
    }
}
```

> 上面示意是为了说明状态链，真实项目里可给 `CommitmentEvent` 增加 `Meta` 字段保存 `TicketId + TripId`。

---

## 5) “买票触发”最小入口（蓝图可调用）

### `TravelFacade.h`

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "WorldSimTypes.h"
#include "TravelFacade.generated.h"

UCLASS()
class WORLDSIMDEMO_API UTravelFacade : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="WorldSim|Travel")
    FGuid RequestVacationTrip(FGuid PersonId, int32 FromRegionId, int32 ToRegionId, FDateTime LeaveAt, float Budget, bool bNeedReturn);

private:
    FGuid CreateFallbackTicketOrNull(FGuid PersonId, int32 FromRegionId, int32 ToRegionId, FDateTime LeaveAt) const;
};
```

### `TravelFacade.cpp`

```cpp
#include "TravelFacade.h"
#include "WorldTransportGraphSubsystem.h"
#include "TravelServiceSubsystem.h"
#include "CommitmentSubsystem.h"

FGuid UTravelFacade::RequestVacationTrip(FGuid PersonId, int32 FromRegionId, int32 ToRegionId, FDateTime LeaveAt, float Budget, bool bNeedReturn)
{
    UWorldTransportGraphSubsystem* Graph = GetGameInstance()->GetSubsystem<UWorldTransportGraphSubsystem>();
    UTravelServiceSubsystem* Travel = GetGameInstance()->GetSubsystem<UTravelServiceSubsystem>();
    UCommitmentSubsystem* Commitment = GetGameInstance()->GetSubsystem<UCommitmentSubsystem>();
    if (!Graph || !Travel || !Commitment)
    {
        return FGuid();
    }

    const int32 FromNode = Graph->GetNodeForRegion(FromRegionId);
    const int32 ToNode = Graph->GetNodeForRegion(ToRegionId);
    if (FromNode == INDEX_NONE || ToNode == INDEX_NONE)
    {
        return FGuid();
    }

    FTravelRequest R;
    R.RequestId = FGuid::NewGuid();
    R.PersonId = PersonId;
    R.FromRegionId = FromRegionId;
    R.ToRegionId = ToRegionId;
    R.DesiredDeparture = LeaveAt;
    R.bIsLeisure = true;
    R.MaxSpend = Budget;
    R.MaxRisk = 1.0f;

    FTravelPath Path;
    FRegionSnapshot FromSnap = FRegionSnapshot();
    FRegionSnapshot ToSnap = FRegionSnapshot();

    if (!Graph->FindPath(FromNode, ToNode, 2, FromSnap, ToSnap, Path))
    {
        return FGuid();
    }

    if (Path.TotalCost > Budget)
    {
        return FGuid();
    }

    const FTravelTicket T = Travel->BuyTicket(R, Path, LeaveAt);
    FCommitmentEvent C;
    if (!Travel->BindTravelAsCommitment(T, C))
    {
        return FGuid();
    }

    const FGuid CommitmentId = Commitment->CreateCommitment(C);

    if (bNeedReturn)
    {
        // 简单地再创建一个返回承诺；Scheduler 中按到达事件再驱动也可。这里给一个静态返回。
        FCommitmentEvent Ret = C;
        Ret.CommitmentId = FGuid::NewGuid();
        Ret.FromLocationId = ToNode;
        Ret.ToLocationId = FromNode;
        Ret.Type = ECommitmentType::Transit;
        Ret.EarliestStart = C.ExpectedEnd + FTimespan::FromHours(2);
        Ret.LatestStart = Ret.EarliestStart + FTimespan::FromHours(3);
        Ret.ExpectedEnd = Ret.EarliestStart + FTimespan::FromMinutes(Path.TotalMinutes);
        Commitment->CreateCommitment(Ret);
    }

    return CommitmentId;
}

FGuid UTravelFacade::CreateFallbackTicketOrNull(FGuid PersonId, int32 FromRegionId, int32 ToRegionId, FDateTime LeaveAt) const
{
    return FGuid();
}
```

---

## 6) 阶段6验收目标（最小版）

1. 在同一日期，给玩家创建两个不同目的地的休闲旅行请求：
   - 至少一个返回 `CommitmentId` 成功
   - 另一个若超预算/无路失败返回空 GUID
2. Scheduler 触发时生成事件序列：
   - `Boarded -> InTransit -> Arrived`（到达后再有 `Stay/ReturnBoarded/Returned`）
3. Presence 锁确保在 `InTransit` 区间该 NPC 不会被同一区域其他系统同时呈现
4. `RegionSnapshot` 接受在途流量的 `ApplyMicroDelta` 更新
5. 若图边容量不足/拥堵高，返回 `Delayed/Cancelled` 分支（来自阶段5）并且不产生“瞬移”

---

## 7) 接入建议（和你先前文件的衔接点）

- 阶段2 调度器：当承诺类型是 `Tourism/Transit`，优先从 `FCommitmentEvent::RouteId` 读取 `TicketId`/`Route`。
- 阶段3/4 局部人物：可视化时把 `PersonLite.HomeRegionId` 与旅行承诺目标节点对齐，避免出现 NPC 在离线任务中“站错地方”。
- 阶段5 冲突：把拥堵和路线容量异常映射成 `CommitmentConflictKind::Delayed/Missed/Cancelled`，再回写认知层。

> 阶段6完成后，你就能支持“周末买票去旅游”，并把结果体现在真相层和区域快照，且不会破坏一致性。
