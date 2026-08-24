#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WorldSimTypes.h"
#include "WorldTimeSubsystem.generated.h"

UCLASS(BlueprintType)
class WORLDSIMDEMO_API UWorldTimeSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="World Simulation|Time")
    FWorldTime GetCurrentWorldTime() const { return CurrentTime; }

    UFUNCTION(BlueprintCallable, Category="World Simulation|Time")
    void AdvanceMinutes(int32 Minutes);

private:
    UPROPERTY()
    FWorldTime CurrentTime;
};
