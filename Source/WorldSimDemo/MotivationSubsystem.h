#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WorldSimTypes.h"
#include "MotivationSubsystem.generated.h"

UCLASS(BlueprintType)
class WORLDSIMDEMO_API UMotivationSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="World Simulation|Motivation")
    bool SetCausalState(const FPersonCausalState& State);

    UFUNCTION(BlueprintPure, Category="World Simulation|Motivation")
    bool TryGetCausalState(FGuid PersonId, FPersonCausalState& OutState) const;

    UFUNCTION(BlueprintCallable, Category="World Simulation|Motivation")
    FDecisionTrace EvaluateDecision(FGuid PersonId);

    UFUNCTION(BlueprintPure, Category="World Simulation|Motivation")
    bool TryGetDecisionTrace(FGuid PersonId, FDecisionTrace& OutTrace) const;

    UFUNCTION(BlueprintCallable, Category="World Simulation|Motivation")
    void AdvanceCausalStates(int32 Minutes);

    UFUNCTION(BlueprintPure, Category="World Simulation|Motivation")
    TArray<FGuid> GetRegisteredPersonIds() const;

private:
    UPROPERTY()
    TMap<FGuid, FPersonCausalState> States;

    UPROPERTY()
    TMap<FGuid, FDecisionTrace> LastDecisions;

    static void NormalizeState(FPersonCausalState& State);
};
