#include "CommitmentSubsystem.h"
#include "KnowledgeSubsystem.h"
#include "MotivationSubsystem.h"
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
    Commitment.RiskLevel = FMath::Clamp(Commitment.RiskLevel, 0.0f, 1.0f);
    Commitment.Outcome = ECommitmentOutcome::None;
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

TArray<FCommitment> UCommitmentSubsystem::GetCommitments(const int32 MaxResults) const
{
    TArray<FCommitment> Result;
    Commitments.GenerateValueArray(Result);
    Result.Sort([](const FCommitment& A, const FCommitment& B)
    {
        if (A.PlannedStart.Minute != B.PlannedStart.Minute)
        {
            return A.PlannedStart.Minute < B.PlannedStart.Minute;
        }
        return A.CommitmentId.ToString(EGuidFormats::Digits)
            < B.CommitmentId.ToString(EGuidFormats::Digits);
    });

    const int32 BoundedMaxResults = FMath::Clamp(MaxResults, 1, 1000);
    if (Result.Num() > BoundedMaxResults)
    {
        Result.SetNum(BoundedMaxResults);
    }
    return Result;
}

TArray<FCommitment> UCommitmentSubsystem::GetActiveCommitments(const int32 MaxResults) const
{
    TArray<FCommitment> Result;
    for (const TPair<FGuid, FCommitment>& Pair : Commitments)
    {
        const ECommitmentState State = Pair.Value.State;
        if (State != ECommitmentState::Completed
            && State != ECommitmentState::Cancelled
            && State != ECommitmentState::Failed)
        {
            Result.Add(Pair.Value);
        }
    }

    Result.Sort([](const FCommitment& A, const FCommitment& B)
    {
        if (A.PlannedStart.Minute != B.PlannedStart.Minute)
        {
            return A.PlannedStart.Minute < B.PlannedStart.Minute;
        }
        return A.CommitmentId.ToString(EGuidFormats::Digits)
            < B.CommitmentId.ToString(EGuidFormats::Digits);
    });

    const int32 BoundedMaxResults = FMath::Clamp(MaxResults, 1, 1000);
    if (Result.Num() > BoundedMaxResults)
    {
        Result.SetNum(BoundedMaxResults);
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
bool UCommitmentSubsystem::TryGetBlockingCommitment(
    const FGuid SubjectId,
    const FWorldTime ActivityStart,
    const FWorldTime ActivityEnd,
    FCommitment& OutCommitment) const
{
    if (!SubjectId.IsValid() || ActivityEnd <= ActivityStart)
    {
        return false;
    }

    for (const TPair<FGuid, FCommitment>& Pair : Commitments)
    {
        const FCommitment& Commitment = Pair.Value;
        const bool bInactive = Commitment.State == ECommitmentState::Completed
            || Commitment.State == ECommitmentState::Cancelled
            || Commitment.State == ECommitmentState::Failed;
        const bool bOverlaps = ActivityStart < Commitment.PlannedEnd
            && Commitment.PlannedStart < ActivityEnd;

        if (Commitment.SubjectId == SubjectId
            && Commitment.bHardCommitment
            && !bInactive
            && bOverlaps)
        {
            OutCommitment = Commitment;
            return true;
        }
    }

    return false;
}
int32 UCommitmentSubsystem::AdvanceCommitments(const FWorldTime CurrentTime)
{
    UTruthLedgerSubsystem* TruthLedger = nullptr;
    UKnowledgeSubsystem* Knowledge = nullptr;
    if (UWorld* World = GetWorld())
    {
        TruthLedger = World->GetSubsystem<UTruthLedgerSubsystem>();
        Knowledge = World->GetSubsystem<UKnowledgeSubsystem>();
    }

    int32 TransitionCount = 0;
    for (TPair<FGuid, FCommitment>& Pair : Commitments)
    {
        FCommitment& Commitment = Pair.Value;
        const bool bCanAdvance = Commitment.State == ECommitmentState::Planned
            || Commitment.State == ECommitmentState::Confirmed
            || Commitment.State == ECommitmentState::Departed
            || Commitment.State == ECommitmentState::InTransit;
        if (!bCanAdvance)
        {
            continue;
        }

        ECommitmentState NewState = Commitment.State;
        if (!(CurrentTime < Commitment.PlannedEnd))
        {
            const uint32 Seed = HashCombine(
                GetTypeHash(Commitment.CommitmentId),
                GetTypeHash(Commitment.PlannedEnd.Minute));
            FRandomStream Random(static_cast<int32>(Seed));
            const float Roll = Random.FRand();
            const float DeathChance = FMath::Square(Commitment.RiskLevel) * 0.08f;
            const float MissingChance = DeathChance + Commitment.RiskLevel * 0.10f;
            const float InjuryChance = MissingChance + Commitment.RiskLevel * 0.30f;
            const float FailureChance = InjuryChance + Commitment.RiskLevel * 0.20f;

            if (Roll < DeathChance)
            {
                Commitment.Outcome = ECommitmentOutcome::Deceased;
                NewState = ECommitmentState::Failed;
            }
            else if (Roll < MissingChance)
            {
                Commitment.Outcome = ECommitmentOutcome::Missing;
                NewState = ECommitmentState::Failed;
            }
            else if (Roll < InjuryChance)
            {
                Commitment.Outcome = ECommitmentOutcome::Injured;
                NewState = ECommitmentState::Completed;
            }
            else if (Roll < FailureChance)
            {
                Commitment.Outcome = ECommitmentOutcome::Failed;
                NewState = ECommitmentState::Failed;
            }
            else
            {
                Commitment.Outcome = ECommitmentOutcome::Succeeded;
                NewState = ECommitmentState::Completed;
            }
        }
        else if (!(CurrentTime < Commitment.PlannedStart))
        {
            NewState = ECommitmentState::InTransit;
        }

        if (NewState == Commitment.State)
        {
            continue;
        }

        Commitment.State = NewState;
        ++TransitionCount;

        if (UWorld* World = GetWorld())
        {
            if (UMotivationSubsystem* Motivation = World->GetSubsystem<UMotivationSubsystem>())
            {
                FPersonCausalState State;
                if (Motivation->TryGetCausalState(Commitment.SubjectId, State))
                {
                    if (Commitment.Outcome == ECommitmentOutcome::Deceased)
                    {
                        State.LifeState = EPersonLifeState::Deceased;
                        State.Health = 0.0f;
                        State.CurrentRegion = NAME_None;
                    }
                    else if (Commitment.Outcome == ECommitmentOutcome::Missing)
                    {
                        State.LifeState = EPersonLifeState::Missing;
                        State.CurrentRegion = NAME_None;
                    }
                    else
                    {
                        State.CurrentRegion = NewState == ECommitmentState::Completed
                            ? Commitment.DestinationRegion
                            : NAME_None;
                        if (Commitment.Outcome == ECommitmentOutcome::Injured)
                        {
                            State.Health = FMath::Max(0.1f, State.Health - 0.45f * Commitment.RiskLevel);
                            State.Stress = FMath::Min(1.0f, State.Stress + 0.40f * Commitment.RiskLevel);
                        }
                    }
                    Motivation->SetCausalState(State);
                }
            }
        }

        if (TruthLedger != nullptr)
        {
            FWorldEvent Event;
            Event.OccurredAt = CurrentTime;
            Event.EventType = TEXT("CommitmentStateChanged");
            Event.SubjectId = Commitment.SubjectId;
            Event.RegionId = NewState == ECommitmentState::Completed
                ? Commitment.DestinationRegion
                : Commitment.OriginRegion;
            if (Commitment.Outcome == ECommitmentOutcome::Deceased)
            {
                Event.Summary = FString::Printf(TEXT("Subject died during commitment: %s"), *Commitment.CommitmentType.ToString());
            }
            else if (Commitment.Outcome == ECommitmentOutcome::Missing)
            {
                Event.Summary = FString::Printf(TEXT("Subject went missing during commitment: %s"), *Commitment.CommitmentType.ToString());
            }
            else if (Commitment.Outcome == ECommitmentOutcome::Injured)
            {
                Event.Summary = FString::Printf(TEXT("Commitment completed with injury: %s"), *Commitment.CommitmentType.ToString());
            }
            else if (Commitment.Outcome == ECommitmentOutcome::Failed)
            {
                Event.Summary = FString::Printf(TEXT("Commitment failed: %s"), *Commitment.CommitmentType.ToString());
            }
            else if (Commitment.Outcome == ECommitmentOutcome::Succeeded)
            {
                Event.Summary = FString::Printf(TEXT("Commitment completed: %s"), *Commitment.CommitmentType.ToString());
            }
            else
            {
                Event.Summary = FString::Printf(TEXT("Commitment is now in transit: %s"), *Commitment.CommitmentType.ToString());
            }
            const FGuid EventId = TruthLedger->RecordEvent(Event);

            if (Knowledge != nullptr && Commitment.Outcome != ECommitmentOutcome::None)
            {
                FWorldMessage Message;
                Message.RelatedEventId = EventId;
                Message.SourceType = TEXT("OfficialReport");
                Message.ReceivedAt = CurrentTime;
                Message.Confidence = Commitment.Outcome == ECommitmentOutcome::Missing ? 0.75f : 0.95f;
                Message.Content = Event.Summary;
                Knowledge->PublishMessage(Message);
            }
        }
    }

    return TransitionCount;
}
