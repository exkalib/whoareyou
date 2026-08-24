#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WorldSimTypes.h"
#include "WorldSimDemoBootstrapSubsystem.generated.h"

UCLASS(BlueprintType)
class WORLDSIMDEMO_API UWorldSimDemoBootstrapSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="World Simulation|Demo")
    bool CreateDemoWorld(int32 Seed, FDemoWorldIds& OutIds, FString& OutFailureReason);

    UFUNCTION(BlueprintPure, Category="World Simulation|Demo")
    bool IsDemoWorldCreated() const { return bCreated; }

private:
    static FGuid MakeStableGuid(int32 Seed, const FString& Label);

    UPROPERTY()
    bool bCreated = false;
};
