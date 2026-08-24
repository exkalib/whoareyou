#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WorldSimTypes.h"
#include "WorldSimulationSubsystem.generated.h"

UCLASS(BlueprintType)
class WORLDSIMDEMO_API UWorldSimulationSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="World Simulation")
    void AdvanceSimulationMinutes(int32 Minutes);

    UFUNCTION(BlueprintPure, Category="World Simulation")
    bool TryGetActiveActivity(FGuid PersonId, FActiveWorldActivity& OutActivity) const;

    UFUNCTION(BlueprintPure, Category="World Simulation")
    TArray<FActiveWorldActivity> GetActiveActivities(int32 MaxResults = 100) const;

private:
    UPROPERTY()
    TMap<FGuid, FActiveWorldActivity> ActiveActivities;

    static int32 GetDefaultDurationMinutes(EWorldGoalType Goal);
    static void ApplyCompletedActivity(FPersonCausalState& State, const FActiveWorldActivity& Activity);
};
