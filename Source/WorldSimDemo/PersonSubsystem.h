#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WorldSimTypes.h"
#include "PersonSubsystem.generated.h"

UCLASS(BlueprintType)
class WORLDSIMDEMO_API UPersonSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="World Simulation|People")
    FGuid CreatePerson(FPersonLite Person);

    UFUNCTION(BlueprintPure, Category="World Simulation|People")
    bool TryGetPerson(FGuid PersonId, FPersonLite& OutPerson) const;

    UFUNCTION(BlueprintPure, Category="World Simulation|People")
    TArray<FPersonLite> GetPeopleInRegion(FName RegionId) const;

    UFUNCTION(BlueprintPure, Category="World Simulation|People")
    TArray<FPersonLite> GetPeople(int32 MaxResults = 100) const;

private:
    UPROPERTY()
    TMap<FGuid, FPersonLite> People;
};
