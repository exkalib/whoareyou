# 阶段5（防穿帮关键钩子）可贴代码：软硬承诺 + 认知层分离

你已经有了 Day1~4 的底座。  
接下来补上“玩家看到/听到”与“世界真相”分离，和“对话触发承诺（soft/hard）”。

这版目标：
- 对话给出一句“将去前线/出差/旅游”只产生**认知事件**先记成消息。
- 只有经过承认+条件通过后，才固化为**硬承诺**（Presence/Truth约束）。
- 世界真相和认知输出分离，避免穿帮信息影响系统状态。

---

## 1) `Source/WorldSimDemo/Public/KnownledgeTypes.h`

```cpp
#pragma once

#include "CoreMinimal.h"
#include "WorldSimCoreTypes.h"
#include "KnownledgeTypes.generated.h"

UENUM(BlueprintType)
enum class EClaimStrength : uint8
{
    Whispers UMETA(DisplayName = "Whispers"),      // 小道消息
    Reported UMETA(DisplayName = "Reported"),     // 目击/他人口述
    Confirmed UMETA(DisplayName = "Confirmed"),   // 多源一致
    Verified UMETA(DisplayName = "Verified")      // 系统真相
};

USTRUCT(BlueprintType)
struct FPlayerKnowledgeItem
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FGuid FactId = FGuid::NewGuid();

    UPROPERTY(BlueprintReadWrite)
    FGuid SubjectPersonId;

    UPROPERTY(BlueprintReadWrite)
    FString Source; // NPC名/新闻源/玩家自测

    UPROPERTY(BlueprintReadWrite)
    EClaimStrength Strength = EClaimStrength::Whispers;

    UPROPERTY(BlueprintReadWrite)
    FString Content;

    UPROPERTY(BlueprintReadWrite)
    FDateTime ObservedAt = FDateTime::UtcNow();

    UPROPERTY(BlueprintReadWrite)
    float Confidence = 0.15f;

    UPROPERTY(BlueprintReadWrite)
    bool bVerified = false;
};

USTRUCT(BlueprintType)
struct FPlayerKnowledgeState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    TMap<FGuid, TArray<FPlayerKnowledgeItem>> ByPerson;
};

USTRUCT(BlueprintType)
struct FConversationCommitDraft
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FGuid SpeakerId = FGuid();

    UPROPERTY(BlueprintReadWrite)
    FGuid TargetPersonId = FGuid();

    UPROPERTY(BlueprintReadWrite)
    ECommitmentType IntentType = ECommitmentType::WorkShift;

    UPROPERTY(BlueprintReadWrite)
    int32 ToLocationId = INDEX_NONE;

    UPROPERTY(BlueprintReadWrite)
    FDateTime Start = FDateTime::MinValue();

    UPROPERTY(BlueprintReadWrite)
    FDateTime End = FDateTime::MinValue();

    UPROPERTY(BlueprintReadWrite)
    bool bHard = false;

    UPROPERTY(BlueprintReadWrite)
    FString SpokenText;
};
```

---

## 2) `Source/WorldSimDemo/Public/KnowledgeSubsystem.h`

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "KnownledgeTypes.h"
#include "KnowledgeSubsystem.generated.h"

UCLASS()
class WORLDSIMDEMO_API UKnowledgeSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "WorldSim|Knowledge")
    void AddKnowledge(const FPlayerKnowledgeItem& Item);

    UFUNCTION(BlueprintCallable, Category = "WorldSim|Knowledge")
    TArray<FPlayerKnowledgeItem> GetKnowledgeForPerson(FGuid PersonId, int32 MaxCount = 20) const;

    UFUNCTION(BlueprintCallable, Category = "WorldSim|Knowledge")
    void PromoteToVerified(FGuid FactId);

    UFUNCTION(BlueprintPure, Category = "WorldSim|Knowledge")
    FText DescribePersonStatus(FGuid PersonId, EExistenceState TrueState, EExistenceState TruthOverride, bool bUseTruth) const;

private:
    UPROPERTY()
    FPlayerKnowledgeState KnowledgeState;

    UPROPERTY()
    TMap<FGuid, FPlayerKnowledgeItem> AllFacts;
};
```

## `Source/WorldSimDemo/Private/KnowledgeSubsystem.cpp`

```cpp
#include "KnowledgeSubsystem.h"

void UKnowledgeSubsystem::AddKnowledge(const FPlayerKnowledgeItem& Item)
{
    FPlayerKnowledgeItem Copy = Item;
    if (!Copy.FactId.IsValid())
    {
        Copy.FactId = FGuid::NewGuid();
    }
    AllFacts.Add(Copy.FactId, Copy);
    KnowledgeState.ByPerson.FindOrAdd(Copy.SubjectPersonId).Add(Copy);
}

TArray<FPlayerKnowledgeItem> UKnowledgeSubsystem::GetKnowledgeForPerson(FGuid PersonId, int32 MaxCount) const
{
    if (const TArray<FPlayerKnowledgeItem>* Arr = KnowledgeState.ByPerson.Find(PersonId))
    {
        TArray<FPlayerKnowledgeItem> Out = *Arr;
        if (Out.Num() > MaxCount)
        {
            Out.SetNum(MaxCount);
        }
        return Out;
    }
    return {};
}

void UKnowledgeSubsystem::PromoteToVerified(FGuid FactId)
{
    if (FPlayerKnowledgeItem* Found = AllFacts.Find(FactId))
    {
        Found->bVerified = true;
        Found->Strength = EClaimStrength::Verified;
        Found->Confidence = 0.99f;

        TArray<FPlayerKnowledgeItem>& Arr = KnowledgeState.ByPerson.FindOrAdd(Found->SubjectPersonId);
        for (FPlayerKnowledgeItem& It : Arr)
        {
            if (It.FactId == FactId)
            {
                It = *Found;
            }
        }
    }
}

FText UKnowledgeSubsystem::DescribePersonStatus(FGuid PersonId, EExistenceState TrueState, EExistenceState TruthOverride, bool bUseTruth) const
{
    const TArray<FPlayerKnowledgeItem>* Arr = KnowledgeState.ByPerson.Find(PersonId);
    if (!Arr || Arr->Num() == 0)
    {
        if (bUseTruth)
        {
            if (TruthOverride == EExistenceState::Dead) return FText::FromString(TEXT("该人物存在状态已确认：死亡。"));
            if (TruthOverride == EExistenceState::Alive) return FText::FromString(TEXT("该人物存在状态：存活。"));
            return FText::FromString(TEXT("该人物状态未知。"));
        }
        return FText::FromString(TEXT("暂无可靠消息。"));
    }

    const FPlayerKnowledgeItem& Top = Arr->Last();
    switch (Top.Strength)
    {
    case EClaimStrength::Whispers:
        return FText::Format(NSLOCTEXT("WorldSim", "K1", "有传言：{0}"), FText::FromString(Top.Content));
    case EClaimStrength::Reported:
        return FText::Format(NSLOCTEXT("WorldSim", "K2", "目击/转述：{0}"), FText::FromString(Top.Content));
    case EClaimStrength::Confirmed:
        return FText::Format(NSLOCTEXT("WorldSim", "K3", "多人印证：{0}"), FText::FromString(Top.Content));
    case EClaimStrength::Verified:
        return FText::FromString(TEXT("已验证（高可信）："));
    default:
        return FText::FromString(TEXT("信息质量不足。"));
    }
}
```

---

## 3) `Source/WorldSimDemo/Public/ConversationCommitService.h`

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WorldSimCoreTypes.h"
#include "KnownledgeTypes.h"
#include "ConversationCommitService.generated.h"

UCLASS()
class WORLDSIMDEMO_API UConversationCommitService : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "WorldSim|Conversation")
    FGuid SubmitDraft(const FConversationCommitDraft& Draft, FString* RejectReason = nullptr);

    UFUNCTION(BlueprintCallable, Category = "WorldSim|Conversation")
    bool ConfirmDraft(FGuid DraftId, bool bAccept, FString* RejectReason = nullptr);

    UFUNCTION(BlueprintCallable, Category = "WorldSim|Conversation")
    bool CancelDraft(FGuid DraftId);

private:
    USTRUCT()
    struct FDraftRecord
    {
        GENERATED_BODY()

        UPROPERTY()
        FGuid DraftId;

        UPROPERTY()
        FConversationCommitDraft Draft;

        UPROPERTY()
        bool bAccepted = false;

        UPROPERTY()
        bool bHardCommitted = false;
    };

    UPROPERTY()
    TMap<FGuid, FDraftRecord> Drafts;
};
```

## `Source/WorldSimDemo/Private/ConversationCommitService.cpp`

```cpp
#include "ConversationCommitService.h"
#include "PresenceSubsystem.h"
#include "CommitmentSubsystem.h"
#include "TruthLedgerSubsystem.h"
#include "KnowledgeSubsystem.h"

FGuid UConversationCommitService::SubmitDraft(const FConversationCommitDraft& Draft, FString* RejectReason)
{
    if (!Draft.TargetPersonId.IsValid())
    {
        if (RejectReason) *RejectReason = TEXT("目标人物无效");
        return FGuid();
    }

    if (Draft.Start >= Draft.End)
    {
        if (RejectReason) *RejectReason = TEXT("时间窗口非法");
        return FGuid();
    }

    FGuid Id = FGuid::NewGuid();
    FDraftRecord NewRecord;
    NewRecord.DraftId = Id;
    NewRecord.Draft = Draft;
    Drafts.Add(Id, NewRecord);

    return Id;
}

bool UConversationCommitService::ConfirmDraft(FGuid DraftId, bool bAccept, FString* RejectReason)
{
    FDraftRecord* Record = Drafts.Find(DraftId);
    if (!Record)
    {
        if (RejectReason) *RejectReason = TEXT("草稿不存在");
        return false;
    }

    if (!bAccept)
    {
        Drafts.Remove(DraftId);
        return true;
    }

    if (Record->bAccepted)
    {
        if (RejectReason) *RejectReason = TEXT("草稿已确认");
        return false;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        if (RejectReason) *RejectReason = TEXT("世界上下文不可用");
        return false;
    }

    auto* ComSub = World->GetSubsystem<UCommitmentSubsystem>();
    auto* PresenceSub = World->GetSubsystem<UPresenceSubsystem>();
    auto* TruthSub = World->GetSubsystem<UTruthLedgerSubsystem>();
    auto* KnowSub = GetGameInstance()->GetSubsystem<UKnowledgeSubsystem>();

    if (!ComSub || !PresenceSub || !TruthSub || !KnowSub)
    {
        if (RejectReason) *RejectReason = TEXT("依赖子系统缺失");
        return false;
    }

    // 先落“认知信息”
    FPlayerKnowledgeItem K;
    K.SubjectPersonId = Record->Draft.TargetPersonId;
    K.Source = TEXT("Conversation");
    K.Strength = Record->Draft.bHard ? EClaimStrength::Reported : EClaimStrength::Whispers;
    K.Content = Record->Draft.SpokenText.IsEmpty() ? TEXT("与对话中提及未来行程") : Record->Draft.SpokenText;
    K.Confidence = Record->Draft.bHard ? 0.55f : 0.35f;
    K.ObservedAt = FDateTime::UtcNow();
    KnowSub->AddKnowledge(K);

    // 仅硬承诺才写入真相系统
    if (Record->Draft.bHard)
    {
        FCommitmentRecord C;
        C.PersonId = Record->Draft.TargetPersonId;
        C.Type = Record->Draft.IntentType;
        C.EarliestStart = Record->Draft.Start;
        C.LatestStart = Record->Draft.Start;
        C.ExpectedEnd = Record->Draft.End;
        C.FromLocationId = 0;
        C.ToLocationId = Record->Draft.ToLocationId;
        C.bHardCommit = true;
        C.bCancelable = true;

        const FGuid CommitmentId = ComSub->CreateCommitment(C);

        FPresenceInterval P;
        P.PersonId = Record->Draft.TargetPersonId;
        P.StartTime = Record->Draft.Start;
        P.EndTime = Record->Draft.End;
        P.RegionId = Record->Draft.ToLocationId;
        P.CommitmentId = CommitmentId;

        FString PreReject;
        if (!PresenceSub->TryReservePresence(P, &PreReject))
        {
            if (RejectReason) *RejectReason = PreReject;
            ComSub->CancelCommitment(CommitmentId, TEXT("与已有存在区间冲突"));
            return false;
        }

        // 真相事件（可回放）
        FWorldEvent E;
        E.SourceCommitmentId = CommitmentId;
        E.PersonId = Record->Draft.TargetPersonId;
        E.EventType = TEXT("CommitAccepted");
        E.EventTime = FDateTime::UtcNow();
        E.LocationId = Record->Draft.ToLocationId;
        E.PayloadJson = TEXT("{}");
        TruthSub->RecordEvent(E);

        Record->bAccepted = true;
        Record->bHardCommitted = true;
        return true;
    }

    Record->bAccepted = true;
    return true;
}

bool UConversationCommitService::CancelDraft(FGuid DraftId)
{
    return Drafts.Remove(DraftId) > 0;
}
```

---

## 4) `WorldSimBlueprintFunctionLibrary` 追加认知入口

在 `WorldSimBlueprintFunctionLibrary.h` 追加：

```cpp
UFUNCTION(BlueprintCallable, Category = "WorldSim|API", meta = (WorldContext = "WorldContextObject"))
static TArray<FPlayerKnowledgeItem> BS_GetPersonKnowledge(const UObject* WorldContextObject, FGuid PersonId, int32 MaxCount = 20);

UFUNCTION(BlueprintCallable, Category = "WorldSim|API", meta = (WorldContext = "WorldContextObject"))
static FGuid BS_SubmitConversationDraft(const UObject* WorldContextObject, const FConversationCommitDraft& Draft, FString& RejectReason);

UFUNCTION(BlueprintCallable, Category = "WorldSim|API", meta = (WorldContext = "WorldContextObject"))
static bool BS_ConfirmConversationDraft(const UObject* WorldContextObject, FGuid DraftId, bool bAccept, FString& RejectReason);
```

在 `WorldSimBlueprintFunctionLibrary.cpp` 追加：

```cpp
#include "KnowledgeSubsystem.h"
#include "ConversationCommitService.h"

TArray<FPlayerKnowledgeItem> UWorldSimBlueprintFunctionLibrary::BS_GetPersonKnowledge(const UObject* WorldContextObject, FGuid PersonId, int32 MaxCount)
{
    if (!WorldContextObject)
    {
        return {};
    }
    UGameInstance* GI = GEngine->GetWorldFromContextObjectChecked(WorldContextObject)->GetGameInstance();
    if (!GI) return {};

    if (auto* K = GI->GetSubsystem<UKnowledgeSubsystem>())
    {
        return K->GetKnowledgeForPerson(PersonId, MaxCount);
    }
    return {};
}

FGuid UWorldSimBlueprintFunctionLibrary::BS_SubmitConversationDraft(const UObject* WorldContextObject, const FConversationCommitDraft& Draft, FString& RejectReason)
{
    if (!WorldContextObject)
    {
        RejectReason = TEXT("WorldContext null");
        return FGuid();
    }
    UWorld* World = GEngine->GetWorldFromContextObjectChecked(WorldContextObject);
    if (!World)
    {
        RejectReason = TEXT("world null");
        return FGuid();
    }

    if (auto* S = World->GetSubsystem<UConversationCommitService>())
    {
        return S->SubmitDraft(Draft, &RejectReason);
    }
    RejectReason = TEXT("Conversation service not exist");
    return FGuid();
}

bool UWorldSimBlueprintFunctionLibrary::BS_ConfirmConversationDraft(const UObject* WorldContextObject, FGuid DraftId, bool bAccept, FString& RejectReason)
{
    if (!WorldContextObject)
    {
        RejectReason = TEXT("WorldContext null");
        return false;
    }
    UWorld* World = GEngine->GetWorldFromContextObjectChecked(WorldContextObject);
    if (!World)
    {
        RejectReason = TEXT("world null");
        return false;
    }

    if (auto* S = World->GetSubsystem<UConversationCommitService>())
    {
        return S->ConfirmDraft(DraftId, bAccept, &RejectReason);
    }
    RejectReason = TEXT("Conversation service not exist");
    return false;
}
```

---

## 5) 阶段5 最小验收（建议今天就跑）

1. 提交一个软承诺草稿（bHard=false）  
   - 只产生 `Knowledge` 记录（`EClaimStrength::Whispers/Reported`）  
   - 不会改变 Presence，不会立刻限制在场一致性
2. 提交一个硬承诺草稿（bHard=true）  
   - 成功时生成 `CommitmentId` 并在 Presence 上成功 reserve  
   - 产生 `TruthLedger` 事件 `CommitAccepted`
3. 对同一玩家“同一时段硬承诺”再次提交，Presence 冲突返回 reject。
4. `BS_GetPersonKnowledge` 可以返回并显示未验证/高可信不同文本来源。
5. 将该人真相设置为 `Dead` 后，`BS_CanPersonExist` 返回 false。
