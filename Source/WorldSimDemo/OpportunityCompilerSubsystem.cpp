#include "OpportunityCompilerSubsystem.h"
#include "MotivationSubsystem.h"
#include "WorldTimeSubsystem.h"

FGuid UOpportunityCompilerSubsystem::RegisterOpportunity(FWorldOpportunity Opportunity)
{
    if (Opportunity.OpportunityType.IsNone() || Opportunity.ExpiresAt <= Opportunity.AvailableFrom)
    {
        return FGuid();
    }
    if (!Opportunity.OpportunityId.IsValid())
    {
        Opportunity.OpportunityId = FGuid::NewGuid();
    }
    Opportunity.Urgency = FMath::Clamp(Opportunity.Urgency, 0.0f, 1.0f);
    Opportunity.Danger = FMath::Clamp(Opportunity.Danger, 0.0f, 1.0f);
    Opportunities.Add(Opportunity.OpportunityId, Opportunity);
    return Opportunity.OpportunityId;
}

bool UOpportunityCompilerSubsystem::RemoveOpportunity(const FGuid OpportunityId)
{
    return Opportunities.Remove(OpportunityId) > 0;
}

TArray<FWorldOpportunity> UOpportunityCompilerSubsystem::GetRelevantOpportunities(const FGuid PersonId, const FName RegionId, const int32 MaxResults)
{
    TArray<FWorldOpportunity> Result;
    UWorld* World = GetWorld();
    if (World == nullptr || MaxResults <= 0)
    {
        return Result;
    }

    const UWorldTimeSubsystem* Time = World->GetSubsystem<UWorldTimeSubsystem>();
    UMotivationSubsystem* Motivation = World->GetSubsystem<UMotivationSubsystem>();
    if (Time == nullptr || Motivation == nullptr)
    {
        return Result;
    }

    const FWorldTime Now = Time->GetCurrentWorldTime();
    const FDecisionTrace Decision = Motivation->EvaluateDecision(PersonId);
    FPersonCausalState State;
    Motivation->TryGetCausalState(PersonId, State);

    struct FScoredOpportunity
    {
        FWorldOpportunity Opportunity;
        float Score = 0.0f;
    };

    TArray<FScoredOpportunity> Scored;
    for (const TPair<FGuid, FWorldOpportunity>& Pair : Opportunities)
    {
        const FWorldOpportunity& Opportunity = Pair.Value;
        if (!Opportunity.IsAvailableAt(Now))
        {
            continue;
        }

        float Score = Opportunity.Urgency * 50.0f;
        if (Opportunity.RegionId == RegionId)
        {
            Score += 25.0f;
        }
        if (Opportunity.SupportedGoals.Contains(Decision.ChosenGoal))
        {
            Score += 100.0f;
        }
        if (Decision.ChosenGoal == EWorldGoalType::Work && State.IncomePressure > 0.0f)
        {
            Score += FMath::Min(25.0f, Opportunity.CreditReward * 0.01f * State.IncomePressure);
        }
        Score -= Opportunity.Danger * (1.0f - State.RiskTolerance) * 40.0f;

        FScoredOpportunity Entry;
        Entry.Opportunity = Opportunity;
        Entry.Score = Score;
        Scored.Add(Entry);
    }

    Scored.Sort([](const FScoredOpportunity& A, const FScoredOpportunity& B)
    {
        return A.Score > B.Score;
    });

    const int32 Count = FMath::Min(MaxResults, Scored.Num());
    for (int32 Index = 0; Index < Count; ++Index)
    {
        Result.Add(Scored[Index].Opportunity);
    }
    return Result;
}
