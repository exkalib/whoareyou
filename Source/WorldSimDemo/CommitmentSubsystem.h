#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WorldSimTypes.h"
#include "CommitmentSubsystem.generated.h"

UCLASS(BlueprintType)
class WORLDSIMDEMO_API UCommitmentSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="World Simulation|Commitment")
    FGuid CreateCommitment(FCommitment Commitment);

    UFUNCTION(BlueprintCallable, Category="World Simulation|Commitment")
    bool TransitionCommitment(FGuid CommitmentId, ECommitmentState NewState);

    UFUNCTION(BlueprintPure, Category="World Simulation|Commitment")
    bool TryGetCommitment(FGuid CommitmentId, FCommitment& OutCommitment) const;

    UFUNCTION(BlueprintCallable, Category="World Simulation|Commitment")
    TArray<FCommitment> GetCommitmentsForSubject(FGuid SubjectId) const;

private:
    UPROPERTY()
    TMap<FGuid, FCommitment> Commitments;

    bool IsValidTransition(ECommitmentState From, ECommitmentState To) const;
};
