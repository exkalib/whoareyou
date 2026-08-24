#include "MotivationSubsystem.h"
#include "WorldTimeSubsystem.h"

bool UMotivationSubsystem::SetCausalState(const FPersonCausalState& State)
{
    if (!State.PersonId.IsValid())
    {
        return false;
    }

    FPersonCausalState StoredState = State;
    NormalizeState(StoredState);
    States.Add(StoredState.PersonId, StoredState);
    return true;
}

bool UMotivationSubsystem::TryGetCausalState(const FGuid PersonId, FPersonCausalState& OutState) const
{
    if (const FPersonCausalState* State = States.Find(PersonId))
    {
        OutState = *State;
        return true;
    }
    return false;
}

FDecisionTrace UMotivationSubsystem::EvaluateDecision(const FGuid PersonId)
{
    FDecisionTrace Trace;
    Trace.PersonId = PersonId;

    const UWorld* World = GetWorld();
    if (World != nullptr)
    {
        if (const UWorldTimeSubsystem* Time = World->GetSubsystem<UWorldTimeSubsystem>())
        {
            Trace.EvaluatedAt = Time->GetCurrentWorldTime();
        }
    }

    const FPersonCausalState* State = States.Find(PersonId);
    if (State == nullptr)
    {
        Trace.ChosenReason = TEXT("No causal state is registered for this person.");
        return Trace;
    }

    auto AddCandidate = [&Trace](const EWorldGoalType Goal, const float Utility, const FString& Reason)
    {
        FDecisionCandidate Candidate;
        Candidate.Goal = Goal;
        Candidate.Utility = FMath::Max(0.0f, Utility);
        Candidate.Reason = Reason;
        Trace.Candidates.Add(Candidate);
    };

    AddCandidate(EWorldGoalType::SeekMedicalCare,
        (1.0f - State->Health) * 130.0f + State->Stress * 15.0f,
        FString::Printf(TEXT("Health is %.0f%% and medical risk is increasing."), State->Health * 100.0f));

    AddCandidate(EWorldGoalType::Eat,
        State->Hunger * 100.0f + (State->Credits <= 0.0f ? -15.0f : 0.0f),
        FString::Printf(TEXT("Hunger is %.0f%%; available credits affect food options."), State->Hunger * 100.0f));

    AddCandidate(EWorldGoalType::Rest,
        State->Fatigue * 95.0f + State->Stress * 20.0f,
        FString::Printf(TEXT("Fatigue is %.0f%% and stress is %.0f%%."), State->Fatigue * 100.0f, State->Stress * 100.0f));

    AddCandidate(EWorldGoalType::Work,
        State->IncomePressure * 55.0f + State->WorkObligation * 65.0f + State->FamilyPressure * 25.0f - State->Fatigue * 20.0f,
        FString::Printf(TEXT("Income pressure is %.0f%% and work obligation is %.0f%%."), State->IncomePressure * 100.0f, State->WorkObligation * 100.0f));

    AddCandidate(EWorldGoalType::Socialize,
        State->Loneliness * 65.0f - State->Stress * 10.0f,
        FString::Printf(TEXT("Loneliness is %.0f%%."), State->Loneliness * 100.0f));

    AddCandidate(EWorldGoalType::Investigate,
        State->Curiosity * 55.0f + State->TrainingNeed * 25.0f - (1.0f - State->RiskTolerance) * 20.0f,
        FString::Printf(TEXT("Curiosity is %.0f%% and risk tolerance is %.0f%%."), State->Curiosity * 100.0f, State->RiskTolerance * 100.0f));

    AddCandidate(EWorldGoalType::Train,
        State->TrainingNeed * 80.0f + State->Curiosity * 10.0f - State->Fatigue * 25.0f,
        FString::Printf(TEXT("Training need is %.0f%%; fatigue limits safe progress."), State->TrainingNeed * 100.0f));

    Trace.Candidates.Sort([](const FDecisionCandidate& A, const FDecisionCandidate& B)
    {
        return A.Utility > B.Utility;
    });

    if (!Trace.Candidates.IsEmpty())
    {
        Trace.ChosenGoal = Trace.Candidates[0].Goal;
        Trace.ChosenReason = Trace.Candidates[0].Reason;
    }

    LastDecisions.Add(PersonId, Trace);
    return Trace;
}

bool UMotivationSubsystem::TryGetDecisionTrace(const FGuid PersonId, FDecisionTrace& OutTrace) const
{
    if (const FDecisionTrace* Trace = LastDecisions.Find(PersonId))
    {
        OutTrace = *Trace;
        return true;
    }
    return false;
}

void UMotivationSubsystem::AdvanceCausalStates(const int32 Minutes)
{
    if (Minutes <= 0)
    {
        return;
    }

    const float Hours = static_cast<float>(Minutes) / 60.0f;
    for (TPair<FGuid, FPersonCausalState>& Entry : States)
    {
        FPersonCausalState& State = Entry.Value;
        State.Hunger = FMath::Clamp(State.Hunger + Hours * 0.04f, 0.0f, 1.0f);
        State.Fatigue = FMath::Clamp(State.Fatigue + Hours * 0.025f, 0.0f, 1.0f);

        if (State.Hunger >= 0.95f || State.Fatigue >= 0.98f)
        {
            State.Health = FMath::Clamp(State.Health - Hours * 0.01f, 0.0f, 1.0f);
        }
    }
}

TArray<FGuid> UMotivationSubsystem::GetRegisteredPersonIds() const
{
    TArray<FGuid> PersonIds;
    States.GetKeys(PersonIds);
    return PersonIds;
}

void UMotivationSubsystem::NormalizeState(FPersonCausalState& State)
{
    State.Hunger = FMath::Clamp(State.Hunger, 0.0f, 1.0f);
    State.Fatigue = FMath::Clamp(State.Fatigue, 0.0f, 1.0f);
    State.Health = FMath::Clamp(State.Health, 0.0f, 1.0f);
    State.Stress = FMath::Clamp(State.Stress, 0.0f, 1.0f);
    State.Loneliness = FMath::Clamp(State.Loneliness, 0.0f, 1.0f);
    State.IncomePressure = FMath::Clamp(State.IncomePressure, 0.0f, 1.0f);
    State.WorkObligation = FMath::Clamp(State.WorkObligation, 0.0f, 1.0f);
    State.FamilyPressure = FMath::Clamp(State.FamilyPressure, 0.0f, 1.0f);
    State.Curiosity = FMath::Clamp(State.Curiosity, 0.0f, 1.0f);
    State.RiskTolerance = FMath::Clamp(State.RiskTolerance, 0.0f, 1.0f);
    State.TrainingNeed = FMath::Clamp(State.TrainingNeed, 0.0f, 1.0f);
}
