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

    UFUNCTION(BlueprintPure, Category="World Simulation|Commitment")
    TArray<FCommitment> GetCommitmentsForSubject(FGuid SubjectId) const;

    UFUNCTION(BlueprintPure, Category="World Simulation|Commitment")
    TArray<FCommitment> GetCommitments(int32 MaxResults = 100) const;

    UFUNCTION(BlueprintPure, Category="World Simulation|Commitment")
    TArray<FCommitment> GetActiveCommitments(int32 MaxResults = 100) const;

    UFUNCTION(BlueprintPure, Category="World Simulation|Commitment")
    bool TryGetBlockingCommitment(
        FGuid SubjectId,
        FWorldTime ActivityStart,
        FWorldTime ActivityEnd,
        FCommitment& OutCommitment) const;

    UFUNCTION(BlueprintCallable, Category="World Simulation|Commitment")
    int32 AdvanceCommitments(FWorldTime CurrentTime);

private:
    UPROPERTY()
    TMap<FGuid, FCommitment> Commitments;

    bool IsValidTransition(ECommitmentState From, ECommitmentState To) const;
};
