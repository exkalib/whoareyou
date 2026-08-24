#pragma once

#include "CoreMinimal.h"
#include "WorldSimTypes.h"
#include "WorldSimDebugTypes.generated.h"

USTRUCT(BlueprintType)
struct WORLDSIMDEMO_API FPersonDebugSnapshot
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    FPersonLite Person;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    FPersonCausalState CausalState;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    EWorldGoalType ChosenGoal = EWorldGoalType::None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    FString ChosenReason;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    FGuid ActiveOpportunityId;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TArray<FGuid> ActiveCommitmentIds;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 KnownMessageCount = 0;
};

USTRUCT(BlueprintType)
struct WORLDSIMDEMO_API FWorldDebugSnapshot
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    FWorldTime WorldTime;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TArray<FPersonDebugSnapshot> People;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TArray<FActiveWorldActivity> Activities;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TArray<FCommitment> Commitments;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TArray<FWorldEvent> RecentEvents;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 PublicMessageCount = 0;
};
