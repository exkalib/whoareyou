#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WorldSimDebugTypes.h"
#include "WorldSimDebugSubsystem.generated.h"

UCLASS(BlueprintType)
class WORLDSIMDEMO_API UWorldSimDebugSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category="World Simulation|Debug")
    FWorldDebugSnapshot BuildSnapshot(const FSimulationDebugLimits& Limits) const;
};
