#include "DialogueSubsystem.h"
#include "CommitmentSubsystem.h"

FGuid UDialogueSubsystem::CreateDialogueCommitment(
    const FGuid SubjectId,
    const FName CommitmentType,
    const FName OriginRegion,
    const FName DestinationRegion,
    const FWorldTime PlannedStart,
    const FWorldTime PlannedEnd,
    const bool bHardCommitment)
{
    if (const UWorld* World = GetWorld())
    {
        if (UCommitmentSubsystem* Commitments = World->GetSubsystem<UCommitmentSubsystem>())
        {
            FCommitment Commitment;
            Commitment.SubjectId = SubjectId;
            Commitment.CommitmentType = CommitmentType;
            Commitment.OriginRegion = OriginRegion;
            Commitment.DestinationRegion = DestinationRegion;
            Commitment.PlannedStart = PlannedStart;
            Commitment.PlannedEnd = PlannedEnd;
            Commitment.bHardCommitment = bHardCommitment;
            return Commitments->CreateCommitment(Commitment);
        }
    }
    return FGuid();
}
