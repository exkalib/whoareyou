#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WorldSimTypes.h"
#include "PresenceSubsystem.generated.h"

UCLASS(BlueprintType)
class WORLDSIMDEMO_API UPresenceSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="World Simulation|Presence")
    bool RegisterInterval(const FPresenceInterval& Interval);

    UFUNCTION(BlueprintCallable, Category="World Simulation|Presence")
    bool CanExistAt(FGuid PersonId, FName RegionId, FWorldTime Time) const;

    UFUNCTION(BlueprintCallable, Category="World Simulation|Presence")
    EWorldPresenceState GetStateAt(FGuid PersonId, FName RegionId, FWorldTime Time) const;

private:
    UPROPERTY()
    TArray<FPresenceInterval> Intervals;
};
