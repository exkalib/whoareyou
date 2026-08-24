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

private:
    UPROPERTY()
    TArray<FWorldMessage> Messages;
};
