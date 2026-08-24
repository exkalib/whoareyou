#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WorldSimTypes.h"
#include "RegionSnapshotSubsystem.generated.h"

UCLASS(BlueprintType)
class WORLDSIMDEMO_API URegionSnapshotSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="World Simulation|Region")
    FRegionSnapshot GetSnapshot(FName RegionId) const;

    UFUNCTION(BlueprintCallable, Category="World Simulation|Region")
    void SetSnapshot(const FRegionSnapshot& Snapshot);

private:
    UPROPERTY()
    TMap<FName, FRegionSnapshot> Snapshots;
};
