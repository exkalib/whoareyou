#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WorldSimTypes.h"
#include "LocalPersonManagerSubsystem.generated.h"

UCLASS(BlueprintType)
class WORLDSIMDEMO_API ULocalPersonManagerSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="World Simulation|People")
    bool InstantiatePerson(FGuid PersonId, FName RegionId, FWorldTime Time, FPersonFull& OutPerson);

    UFUNCTION(BlueprintCallable, Category="World Simulation|People")
    TArray<FGuid> RefreshVisiblePeople(FName RegionId, FWorldTime Time, int32 MaxVisiblePeople);

    UFUNCTION(BlueprintCallable, Category="World Simulation|People")
    void RecyclePerson(FGuid PersonId);

    UFUNCTION(BlueprintPure, Category="World Simulation|People")
    TArray<FGuid> GetInstantiatedPeople() const;

    UFUNCTION(BlueprintCallable, Category="World Simulation|People")
    bool IsPersonAvailable(FGuid PersonId, FName RegionId, FWorldTime Time) const;

    UFUNCTION(BlueprintCallable, Category="World Simulation|People")
    void SetRegion(FName RegionId);

    UFUNCTION(BlueprintPure, Category="World Simulation|People")
    FName GetActiveRegion() const { return ActiveRegion; }

private:
    UPROPERTY()
    FName ActiveRegion;

    UPROPERTY()
    TMap<FGuid, FPersonFull> ActivePeople;
};
