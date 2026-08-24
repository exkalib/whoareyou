#include "WorldSimulationSubsystem.h"

#include "CommitmentSubsystem.h"
#include "MotivationSubsystem.h"
#include "OpportunityCompilerSubsystem.h"
#include "PersonSubsystem.h"
#include "TruthLedgerSubsystem.h"
#include "WorldTimeSubsystem.h"

void UWorldSimulationSubsystem::AdvanceSimulationMinutes(const int32 Minutes)
{
    UWorld* World = GetWorld();
    if (Minutes <= 0 || World == nullptr)
    {
        return;
    }

    UWorldTimeSubsystem* WorldTime = World->GetSubsystem<UWorldTimeSubsystem>();
    UCommitmentSubsystem* Commitments = World->GetSubsystem<UCommitmentSubsystem>();
    UMotivationSubsystem* Motivation = World->GetSubsystem<UMotivationSubsystem>();
    UPersonSubsystem* People = World->GetSubsystem<UPersonSubsystem>();
    UOpportunityCompilerSubsystem* Opportunities = World->GetSubsystem<UOpportunityCompilerSubsystem>();
    UTruthLedgerSubsystem* TruthLedger = World->GetSubsystem<UTruthLedgerSubsystem>();
    if (WorldTime == nullptr || Commitments == nullptr || Motivation == nullptr || People == nullptr || Opportunities == nullptr || TruthLedger == nullptr)
    {
        return;
    }

    WorldTime->AdvanceMinutes(Minutes);
    Commitments->AdvanceCommitments(WorldTime->GetCurrentWorldTime());
    Motivation->AdvanceCausalStates(Minutes);

    TArray<FGuid> CompletedPersonIds;
    for (TPair<FGuid, FActiveWorldActivity>& Entry : ActiveActivities)
    {
        FActiveWorldActivity& Activity = Entry.Value;
        Activity.RemainingMinutes -= Minutes;
        if (Activity.RemainingMinutes > 0)
        {
            continue;
        }

        FPersonCausalState State;
        if (Motivation->TryGetCausalState(Activity.PersonId, State))
        {
            ApplyCompletedActivity(State, Activity);
            Motivation->SetCausalState(State);
        }

        FWorldEvent Event;
        Event.EventType = TEXT("ActivityCompleted");
        Event.SubjectId = Activity.PersonId;
        Event.RegionId = Activity.RegionId;
        Event.Summary = FString::Printf(TEXT("Completed: %s"), *Activity.Title);
        Event.OccurredAt = WorldTime->GetCurrentWorldTime();
        TruthLedger->RecordEvent(Event);
        CompletedPersonIds.Add(Activity.PersonId);
    }

    for (const FGuid& PersonId : CompletedPersonIds)
    {
        ActiveActivities.Remove(PersonId);
    }

    for (const FGuid& PersonId : Motivation->GetRegisteredPersonIds())
    {
        FPersonCausalState State;
        FPersonLite Person;
        if (!Motivation->TryGetCausalState(PersonId, State)
            || !State.bAutonomous
            || State.LifeState != EPersonLifeState::Active
            || !People->TryGetPerson(PersonId, Person))
        {
            continue;
        }

        if (ActiveActivities.Contains(PersonId))
        {
            continue;
        }

        const FDecisionTrace Decision = Motivation->EvaluateDecision(PersonId);

        FWorldOpportunity ClaimedOpportunity;
        const FName DecisionRegion = State.CurrentRegion.IsNone()
            ? Person.HomeRegion
            : State.CurrentRegion;
        if (!Opportunities->TryClaimBestOpportunity(PersonId, DecisionRegion, ClaimedOpportunity))
        {
            continue;
        }

        FActiveWorldActivity Activity;
        Activity.PersonId = PersonId;
        Activity.OpportunityId = ClaimedOpportunity.OpportunityId;
        Activity.Title = ClaimedOpportunity.Summary;
        Activity.RegionId = ClaimedOpportunity.RegionId;
        Activity.Goal = Decision.ChosenGoal;
        Activity.RemainingMinutes = ClaimedOpportunity.DurationMinutes > 0
            ? ClaimedOpportunity.DurationMinutes
            : GetDefaultDurationMinutes(Decision.ChosenGoal);
        Activity.CreditReward = ClaimedOpportunity.CreditReward;
        Activity.Danger = ClaimedOpportunity.Danger;
        ActiveActivities.Add(PersonId, Activity);

        FWorldEvent Event;
        Event.EventType = TEXT("OpportunityClaimed");
        Event.SubjectId = PersonId;
        Event.RegionId = ClaimedOpportunity.RegionId;
        Event.Summary = FString::Printf(TEXT("%s chose: %s"), *Person.DisplayName, *ClaimedOpportunity.Summary);
        Event.OccurredAt = WorldTime->GetCurrentWorldTime();
        TruthLedger->RecordEvent(Event);
    }
}

bool UWorldSimulationSubsystem::TryGetActiveActivity(const FGuid PersonId, FActiveWorldActivity& OutActivity) const
{
    if (const FActiveWorldActivity* Activity = ActiveActivities.Find(PersonId))
    {
        OutActivity = *Activity;
        return true;
    }
    return false;
}

TArray<FActiveWorldActivity> UWorldSimulationSubsystem::GetActiveActivities(const int32 MaxResults) const
{
    TArray<FActiveWorldActivity> Result;
    ActiveActivities.GenerateValueArray(Result);
    Result.Sort([](const FActiveWorldActivity& A, const FActiveWorldActivity& B)
    {
        if (A.RemainingMinutes != B.RemainingMinutes)
        {
            return A.RemainingMinutes < B.RemainingMinutes;
        }
        return A.PersonId.ToString(EGuidFormats::Digits)
            < B.PersonId.ToString(EGuidFormats::Digits);
    });

    const int32 BoundedMaxResults = FMath::Clamp(MaxResults, 1, 1000);
    if (Result.Num() > BoundedMaxResults)
    {
        Result.SetNum(BoundedMaxResults);
    }
    return Result;
}

int32 UWorldSimulationSubsystem::GetDefaultDurationMinutes(const EWorldGoalType Goal)
{
    switch (Goal)
    {
    case EWorldGoalType::Eat:
        return 30;
    case EWorldGoalType::Rest:
        return 480;
    case EWorldGoalType::Work:
        return 480;
    case EWorldGoalType::SeekMedicalCare:
        return 120;
    case EWorldGoalType::Socialize:
        return 90;
    case EWorldGoalType::Investigate:
        return 180;
    case EWorldGoalType::Train:
        return 240;
    default:
        return 60;
    }
}

void UWorldSimulationSubsystem::ApplyCompletedActivity(FPersonCausalState& State, const FActiveWorldActivity& Activity)
{
    State.Credits = FMath::Max(0.0f, State.Credits + Activity.CreditReward);
    State.Health = FMath::Max(0.0f, State.Health - Activity.Danger * 0.08f);
    State.Stress = FMath::Min(1.0f, State.Stress + Activity.Danger * 0.15f);

    switch (Activity.Goal)
    {
    case EWorldGoalType::Eat:
        State.Hunger = FMath::Max(0.0f, State.Hunger - 0.75f);
        break;
    case EWorldGoalType::Rest:
        State.Fatigue = FMath::Max(0.0f, State.Fatigue - 0.85f);
        State.Stress = FMath::Max(0.0f, State.Stress - 0.25f);
        break;
    case EWorldGoalType::Work:
        State.IncomePressure = FMath::Max(0.0f, State.IncomePressure - 0.20f);
        State.Fatigue = FMath::Min(1.0f, State.Fatigue + 0.20f);
        break;
    case EWorldGoalType::SeekMedicalCare:
        State.Health = FMath::Min(1.0f, State.Health + 0.45f);
        break;
    case EWorldGoalType::Socialize:
        State.Loneliness = FMath::Max(0.0f, State.Loneliness - 0.60f);
        State.Stress = FMath::Max(0.0f, State.Stress - 0.15f);
        break;
    case EWorldGoalType::Investigate:
        State.Curiosity = FMath::Max(0.0f, State.Curiosity - 0.35f);
        break;
    case EWorldGoalType::Train:
        State.TrainingNeed = FMath::Max(0.0f, State.TrainingNeed - 0.50f);
        State.Fatigue = FMath::Min(1.0f, State.Fatigue + 0.25f);
        break;
    default:
        break;
    }
}
