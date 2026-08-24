#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WorldSimTypes.h"
#include "KnowledgeSubsystem.generated.h"

UCLASS(BlueprintType)
class WORLDSIMDEMO_API UKnowledgeSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="World Simulation|Knowledge")
    FGuid PublishMessage(FWorldMessage Message);

    UFUNCTION(BlueprintPure, Category="World Simulation|Knowledge")
    TArray<FWorldMessage> GetMessagesForRecipient(FGuid RecipientId) const;

    UFUNCTION(BlueprintPure, Category="World Simulation|Knowledge")
    TArray<FWorldMessage> GetMessagesAboutEvent(FGuid RelatedEventId) const;

    UFUNCTION(BlueprintPure, Category="World Simulation|Knowledge")
    TArray<FWorldMessage> GetPublicMessages() const;

    UFUNCTION(BlueprintCallable, Category="World Simulation|Knowledge")
    bool LearnMessage(FGuid KnowerId, FGuid MessageId, FWorldTime LearnedAt, float BeliefConfidence);

    UFUNCTION(BlueprintPure, Category="World Simulation|Knowledge")
    TArray<FWorldMessage> GetKnownMessages(FGuid KnowerId) const;

    UFUNCTION(BlueprintPure, Category="World Simulation|Knowledge")
    bool KnowsMessage(FGuid KnowerId, FGuid MessageId) const;

    UFUNCTION(BlueprintPure, Category="World Simulation|Knowledge")
    bool TryGetMessage(FGuid MessageId, FWorldMessage& OutMessage) const;

    UFUNCTION(BlueprintPure, Category="World Simulation|Knowledge")
    TArray<FMessageKnowledge> GetKnowledgeRecords(FGuid KnowerId, int32 MaxResults = 100) const;

private:
    UPROPERTY()
    TArray<FWorldMessage> Messages;

    UPROPERTY()
    TArray<FMessageKnowledge> KnowledgeRecords;
};
