#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WorldSimTypes.h"
#include "DialogueSubsystem.generated.h"

UCLASS(BlueprintType)
class WORLDSIMDEMO_API UDialogueSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="World Simulation|Dialogue")
    FGuid CreateDialogueCommitment(
        FGuid SubjectId,
        FName CommitmentType,
        FName OriginRegion,
        FName DestinationRegion,
        FWorldTime PlannedStart,
        FWorldTime PlannedEnd,
        bool bHardCommitment);
};
