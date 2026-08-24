#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WorldSimTypes.h"
#include "DailyRoutineSubsystem.generated.h"

UCLASS(BlueprintType)
class WORLDSIMDEMO_API UDailyRoutineSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="World Simulation|Routine")
    void GenerateDefaultDay(const FPersonLite& Person, FName WorkRegion);

    UFUNCTION(BlueprintPure, Category="World Simulation|Routine")
    TArray<FDailyScheduleEntry> GetSchedule(FGuid PersonId) const;

    UFUNCTION(BlueprintPure, Category="World Simulation|Routine")
    bool TryGetActivityAt(FGuid PersonId, FWorldTime Time, FDailyScheduleEntry& OutEntry) const;

private:
    TMap<FGuid, TArray<FDailyScheduleEntry>> Schedules;
};
