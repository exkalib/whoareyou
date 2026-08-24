#include "OpportunityCompilerSubsystem.h"
#include "CommitmentSubsystem.h"
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
    Opportunity.DurationMinutes = FMath::Max(1, Opportunity.DurationMinutes);
    if (Opportunity.AvailableUses == 0 || Opportunity.AvailableUses < -1)
    {
        return FGuid();
    }
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
    const UCommitmentSubsystem* Commitments = World->GetSubsystem<UCommitmentSubsystem>();
    if (Time == nullptr || Motivation == nullptr || Commitments == nullptr)
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

        if (Opportunity.RegionId != RegionId || !Opportunity.SupportedGoals.Contains(Decision.ChosenGoal))
        {
            continue;
        }

        FCommitment BlockingCommitment;
        const FWorldTime ActivityEnd = Now + FMath::Max(1, Opportunity.DurationMinutes);
        if (Commitments->TryGetBlockingCommitment(PersonId, Now, ActivityEnd, BlockingCommitment))
        {
            continue;
        }

        float Score = Opportunity.Urgency * 50.0f;
        Score += 125.0f;
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

bool UOpportunityCompilerSubsystem::TryClaimBestOpportunity(const FGuid PersonId, const FName RegionId, FWorldOpportunity& OutOpportunity)
{
    const TArray<FWorldOpportunity> Results = GetRelevantOpportunities(PersonId, RegionId, 1);
    if (Results.Num() == 0)
    {
        return false;
    }

    OutOpportunity = Results[0];
    if (FWorldOpportunity* StoredOpportunity = Opportunities.Find(OutOpportunity.OpportunityId))
    {
        if (StoredOpportunity->AvailableUses > 0)
        {
            --StoredOpportunity->AvailableUses;
            if (StoredOpportunity->AvailableUses == 0)
            {
                Opportunities.Remove(OutOpportunity.OpportunityId);
            }
        }
    }
    return true;
}

TArray<FWorldOpportunity> UOpportunityCompilerSubsystem::GetAvailableOpportunities(
    const FWorldTime At,
    const FName RegionId,
    const int32 MaxResults) const
{
    TArray<FWorldOpportunity> Result;
    for (const TPair<FGuid, FWorldOpportunity>& Pair : Opportunities)
    {
        const FWorldOpportunity& Opportunity = Pair.Value;
        if (!Opportunity.IsAvailableAt(At))
        {
            continue;
        }
        if (!RegionId.IsNone() && Opportunity.RegionId != RegionId)
        {
            continue;
        }
        Result.Add(Opportunity);
    }

    Result.Sort([](const FWorldOpportunity& A, const FWorldOpportunity& B)
    {
        if (A.ExpiresAt.Minute != B.ExpiresAt.Minute)
        {
            return A.ExpiresAt.Minute < B.ExpiresAt.Minute;
        }
        if (!FMath::IsNearlyEqual(A.Urgency, B.Urgency))
        {
            return A.Urgency > B.Urgency;
        }
        return A.OpportunityId.ToString(EGuidFormats::Digits)
            < B.OpportunityId.ToString(EGuidFormats::Digits);
    });

    const int32 BoundedMaxResults = FMath::Clamp(MaxResults, 1, 1000);
    if (Result.Num() > BoundedMaxResults)
    {
        Result.SetNum(BoundedMaxResults);
    }
    return Result;
}
