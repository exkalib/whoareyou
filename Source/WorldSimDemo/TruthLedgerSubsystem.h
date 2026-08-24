#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WorldSimTypes.h"
#include "TruthLedgerSubsystem.generated.h"

UCLASS(BlueprintType)
class WORLDSIMDEMO_API UTruthLedgerSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="World Simulation|Truth")
    FGuid RecordEvent(const FWorldEvent& Event);

    UFUNCTION(BlueprintCallable, Category="World Simulation|Truth")
    TArray<FWorldEvent> GetEventsForSubject(FGuid SubjectId) const;

    UFUNCTION(BlueprintPure, Category="World Simulation|Truth")
    TArray<FWorldEvent> QueryEvents(const FWorldEventQuery& Query) const;

    UFUNCTION(BlueprintPure, Category="World Simulation|Truth")
    TArray<FWorldEvent> GetRecentEvents(int32 MaxResults = 100) const;

    UFUNCTION(BlueprintPure, Category="World Simulation|Truth")
    bool TryGetEvent(FGuid EventId, FWorldEvent& OutEvent) const;

private:
    UPROPERTY()
    TArray<FWorldEvent> Events;
};
