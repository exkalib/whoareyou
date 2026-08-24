#include "CommitmentSubsystem.h"
#include "TruthLedgerSubsystem.h"
#include "WorldTimeSubsystem.h"

FGuid UCommitmentSubsystem::CreateCommitment(FCommitment Commitment)
{
    if (!Commitment.CommitmentId.IsValid())
    {
        Commitment.CommitmentId = FGuid::NewGuid();
    }
    if (!Commitment.SubjectId.IsValid() || Commitment.CommitmentType.IsNone() || Commitment.PlannedEnd <= Commitment.PlannedStart)
    {
        return FGuid();
    }
    Commitments.Add(Commitment.CommitmentId, Commitment);

    if (UWorld* World = GetWorld())
    {
        FWorldEvent Event;
        Event.OccurredAt = World->GetSubsystem<UWorldTimeSubsystem>()->GetCurrentWorldTime();
        Event.EventType = TEXT("CommitmentCreated");
        Event.SubjectId = Commitment.SubjectId;
        Event.RegionId = Commitment.OriginRegion;
        Event.Summary = FString::Printf(TEXT("Commitment %s created"), *Commitment.CommitmentId.ToString());
        World->GetSubsystem<UTruthLedgerSubsystem>()->RecordEvent(Event);
    }
    return Commitment.CommitmentId;
}

bool UCommitmentSubsystem::TransitionCommitment(const FGuid CommitmentId, const ECommitmentState NewState)
{
    FCommitment* Commitment = Commitments.Find(CommitmentId);
    if (Commitment == nullptr || !IsValidTransition(Commitment->State, NewState))
    {
        return false;
    }
    Commitment->State = NewState;

    if (UWorld* World = GetWorld())
    {
        FWorldEvent Event;
        Event.OccurredAt = World->GetSubsystem<UWorldTimeSubsystem>()->GetCurrentWorldTime();
        Event.EventType = TEXT("CommitmentStateChanged");
        Event.SubjectId = Commitment->SubjectId;
        Event.RegionId = Commitment->OriginRegion;
        Event.Summary = FString::Printf(TEXT("Commitment %s changed to %s"), *CommitmentId.ToString(), *UEnum::GetValueAsString(NewState));
        World->GetSubsystem<UTruthLedgerSubsystem>()->RecordEvent(Event);
    }
    return true;
}

bool UCommitmentSubsystem::TryGetCommitment(const FGuid CommitmentId, FCommitment& OutCommitment) const
{
    if (const FCommitment* Commitment = Commitments.Find(CommitmentId))
    {
        OutCommitment = *Commitment;
        return true;
    }
    return false;
}

TArray<FCommitment> UCommitmentSubsystem::GetCommitmentsForSubject(const FGuid SubjectId) const
{
    TArray<FCommitment> Result;
    for (const TPair<FGuid, FCommitment>& Pair : Commitments)
    {
        if (Pair.Value.SubjectId == SubjectId)
        {
            Result.Add(Pair.Value);
        }
    }
    return Result;
}

bool UCommitmentSubsystem::IsValidTransition(const ECommitmentState From, const ECommitmentState To) const
{
    if (From == ECommitmentState::Completed || From == ECommitmentState::Cancelled || From == ECommitmentState::Failed)
    {
        return false;
    }
    if (To == ECommitmentState::Cancelled || To == ECommitmentState::Delayed || To == ECommitmentState::Failed)
    {
        return true;
    }
    if (From == ECommitmentState::Planned && To == ECommitmentState::Confirmed)
    {
        return true;
    }
    if (From == ECommitmentState::Confirmed && To == ECommitmentState::Departed)
    {
        return true;
    }
    if (From == ECommitmentState::Departed && To == ECommitmentState::InTransit)
    {
        return true;
    }
    if (From == ECommitmentState::InTransit && To == ECommitmentState::Completed)
    {
        return true;
    }
    if (From == ECommitmentState::Delayed && To == ECommitmentState::Confirmed)
    {
        return true;
    }
    return false;
}
