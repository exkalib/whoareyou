#include "WorldSimDebugSubsystem.h"

#include "CommitmentSubsystem.h"
#include "KnowledgeSubsystem.h"
#include "MotivationSubsystem.h"
#include "PersonSubsystem.h"
#include "TruthLedgerSubsystem.h"
#include "WorldSimulationSubsystem.h"
#include "WorldTimeSubsystem.h"

FWorldDebugSnapshot UWorldSimDebugSubsystem::BuildSnapshot(const FSimulationDebugLimits& Limits) const
{
    FWorldDebugSnapshot Snapshot;
    UWorld* World = GetWorld();
    if (World == nullptr)
    {
        return Snapshot;
    }

    const UWorldTimeSubsystem* Time = World->GetSubsystem<UWorldTimeSubsystem>();
    const UPersonSubsystem* People = World->GetSubsystem<UPersonSubsystem>();
    const UMotivationSubsystem* Motivation = World->GetSubsystem<UMotivationSubsystem>();
    const UWorldSimulationSubsystem* Simulation = World->GetSubsystem<UWorldSimulationSubsystem>();
    const UCommitmentSubsystem* Commitments = World->GetSubsystem<UCommitmentSubsystem>();
    const UTruthLedgerSubsystem* Truth = World->GetSubsystem<UTruthLedgerSubsystem>();
    const UKnowledgeSubsystem* Knowledge = World->GetSubsystem<UKnowledgeSubsystem>();
    if (Time == nullptr || People == nullptr || Motivation == nullptr
        || Simulation == nullptr || Commitments == nullptr || Truth == nullptr
        || Knowledge == nullptr)
    {
        return Snapshot;
    }

    FSimulationDebugLimits BoundedLimits = Limits;
    BoundedLimits.Normalize();

    Snapshot.WorldTime = Time->GetCurrentWorldTime();
    const TArray<FPersonLite> PersonRecords = People->GetPeople(BoundedLimits.MaxPeople);
    Snapshot.Activities = Simulation->GetActiveActivities(BoundedLimits.MaxActivities);
    Snapshot.Commitments = Commitments->GetCommitments(BoundedLimits.MaxCommitments);
    Snapshot.RecentEvents = Truth->GetRecentEvents(BoundedLimits.MaxEvents);
    Snapshot.PublicMessageCount = Knowledge->GetPublicMessages().Num();

    TMap<FGuid, FActiveWorldActivity> ActivityByPerson;
    for (const FActiveWorldActivity& Activity : Snapshot.Activities)
    {
        ActivityByPerson.Add(Activity.PersonId, Activity);
    }

    TMultiMap<FGuid, FGuid> ActiveCommitmentIdsByPerson;
    for (const FCommitment& Commitment : Snapshot.Commitments)
    {
        if (Commitment.State != ECommitmentState::Completed
            && Commitment.State != ECommitmentState::Cancelled
            && Commitment.State != ECommitmentState::Failed)
        {
            ActiveCommitmentIdsByPerson.Add(Commitment.SubjectId, Commitment.CommitmentId);
        }
    }

    Snapshot.People.Reserve(PersonRecords.Num());
    for (const FPersonLite& Person : PersonRecords)
    {
        FPersonDebugSnapshot PersonSnapshot;
        PersonSnapshot.Person = Person;
        Motivation->TryGetCausalState(Person.PersonId, PersonSnapshot.CausalState);

        FDecisionTrace Decision;
        if (Motivation->TryGetDecisionTrace(Person.PersonId, Decision))
        {
            PersonSnapshot.ChosenGoal = Decision.ChosenGoal;
            PersonSnapshot.ChosenReason = Decision.ChosenReason;
        }

        if (const FActiveWorldActivity* Activity = ActivityByPerson.Find(Person.PersonId))
        {
            PersonSnapshot.ActiveOpportunityId = Activity->OpportunityId;
        }

        ActiveCommitmentIdsByPerson.MultiFind(Person.PersonId, PersonSnapshot.ActiveCommitmentIds);
        PersonSnapshot.ActiveCommitmentIds.Sort([](const FGuid& A, const FGuid& B)
        {
            return A.ToString(EGuidFormats::Digits) < B.ToString(EGuidFormats::Digits);
        });
        PersonSnapshot.KnownMessageCount = Knowledge
            ->GetKnowledgeRecords(Person.PersonId, BoundedLimits.MaxMessages).Num();
        Snapshot.People.Add(PersonSnapshot);
    }

    return Snapshot;
}
