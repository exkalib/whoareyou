#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WorldSimTypes.h"
#include "OpportunityCompilerSubsystem.generated.h"

UCLASS(BlueprintType)
class WORLDSIMDEMO_API UOpportunityCompilerSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="World Simulation|Opportunity")
    FGuid RegisterOpportunity(FWorldOpportunity Opportunity);

    UFUNCTION(BlueprintCallable, Category="World Simulation|Opportunity")
    bool RemoveOpportunity(FGuid OpportunityId);

    UFUNCTION(BlueprintCallable, Category="World Simulation|Opportunity")
    TArray<FWorldOpportunity> GetRelevantOpportunities(FGuid PersonId, FName RegionId, int32 MaxResults);

    UFUNCTION(BlueprintCallable, Category="World Simulation|Opportunity")
    bool TryClaimBestOpportunity(FGuid PersonId, FName RegionId, FWorldOpportunity& OutOpportunity);

    UFUNCTION(BlueprintPure, Category="World Simulation|Opportunity")
    TArray<FWorldOpportunity> GetAvailableOpportunities(
        FWorldTime At,
        FName RegionId,
        int32 MaxResults = 100) const;

private:
    UPROPERTY()
    TMap<FGuid, FWorldOpportunity> Opportunities;
};
