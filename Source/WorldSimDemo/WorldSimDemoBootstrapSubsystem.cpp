#include "WorldSimDemoBootstrapSubsystem.h"

#include "CommitmentSubsystem.h"
#include "MotivationSubsystem.h"
#include "OpportunityCompilerSubsystem.h"
#include "PersonSubsystem.h"
#include "RegionSnapshotSubsystem.h"
#include "Misc/SecureHash.h"

namespace WorldSimDemoLabels
{
    constexpr const TCHAR* Player = TEXT("Person.Player");
    constexpr const TCHAR* Worker = TEXT("Person.Worker");
    constexpr const TCHAR* Traveller = TEXT("Person.Traveller");
    constexpr const TCHAR* SafeCommitment = TEXT("Commitment.SafeTravel");
    constexpr const TCHAR* RiskCommitment = TEXT("Commitment.RiskMission");
    constexpr const TCHAR* FoodOpportunity = TEXT("Opportunity.Food");
    constexpr const TCHAR* WorkOpportunity = TEXT("Opportunity.Work");
    constexpr const TCHAR* RestOpportunity = TEXT("Opportunity.Rest");
    constexpr const TCHAR* MedicalOpportunity = TEXT("Opportunity.Medical");
    constexpr const TCHAR* DangerOpportunity = TEXT("Opportunity.DangerInspection");
}

FGuid UWorldSimDemoBootstrapSubsystem::MakeStableGuid(const int32 Seed, const FString& Label)
{
    auto HashToGuid = [](const FString& Input)
    {
        FTCHARToUTF8 Utf8(*Input);
        FMD5 Md5;
        Md5.Update(reinterpret_cast<const uint8*>(Utf8.Get()), Utf8.Length());

        uint8 Digest[16];
        Md5.Final(Digest);
        auto ReadUint32 = [&Digest](const int32 Offset)
        {
            return (static_cast<uint32>(Digest[Offset]) << 24)
                | (static_cast<uint32>(Digest[Offset + 1]) << 16)
                | (static_cast<uint32>(Digest[Offset + 2]) << 8)
                | static_cast<uint32>(Digest[Offset + 3]);
        };
        return FGuid(ReadUint32(0), ReadUint32(4), ReadUint32(8), ReadUint32(12));
    };

    const FString StableInput = FString::Printf(TEXT("%d:%s"), Seed, *Label);
    FGuid Result = HashToGuid(StableInput);
    if (!Result.IsValid())
    {
        Result = HashToGuid(StableInput + TEXT(":NonZero"));
    }
    return Result;
}

bool UWorldSimDemoBootstrapSubsystem::CreateDemoWorld(
    const int32 Seed,
    FDemoWorldIds& OutIds,
    FString& OutFailureReason)
{
    OutIds = FDemoWorldIds();
    OutFailureReason.Reset();

    if (bCreated)
    {
        OutFailureReason = TEXT("AlreadyCreated");
        return false;
    }

    UWorld* World = GetWorld();
    if (World == nullptr)
    {
        OutFailureReason = TEXT("MissingWorld");
        return false;
    }

    URegionSnapshotSubsystem* Regions = World->GetSubsystem<URegionSnapshotSubsystem>();
    if (Regions == nullptr)
    {
        OutFailureReason = TEXT("MissingSubsystem:RegionSnapshot");
        return false;
    }
    UPersonSubsystem* People = World->GetSubsystem<UPersonSubsystem>();
    if (People == nullptr)
    {
        OutFailureReason = TEXT("MissingSubsystem:Person");
        return false;
    }
    UMotivationSubsystem* Motivation = World->GetSubsystem<UMotivationSubsystem>();
    if (Motivation == nullptr)
    {
        OutFailureReason = TEXT("MissingSubsystem:Motivation");
        return false;
    }
    UOpportunityCompilerSubsystem* Opportunities = World->GetSubsystem<UOpportunityCompilerSubsystem>();
    if (Opportunities == nullptr)
    {
        OutFailureReason = TEXT("MissingSubsystem:OpportunityCompiler");
        return false;
    }
    UCommitmentSubsystem* Commitments = World->GetSubsystem<UCommitmentSubsystem>();
    if (Commitments == nullptr)
    {
        OutFailureReason = TEXT("MissingSubsystem:Commitment");
        return false;
    }

    (void)Opportunities;
    (void)Commitments;

    OutIds.RegionId = TEXT("Port_Aster");
    OutIds.PlayerId = MakeStableGuid(Seed, WorldSimDemoLabels::Player);
    OutIds.WorkerId = MakeStableGuid(Seed, WorldSimDemoLabels::Worker);
    OutIds.TravellerId = MakeStableGuid(Seed, WorldSimDemoLabels::Traveller);

    FRegionSnapshot Region;
    Region.RegionId = OutIds.RegionId;
    Region.AtTime = FWorldTime(0);
    Region.VisiblePopulation = 3;
    Region.ActiveJobs = 1;
    Regions->SetSnapshot(Region);

    auto CreatePerson = [People, &OutFailureReason](FPersonLite Person, const TCHAR* Step)
    {
        const FGuid ExpectedId = Person.PersonId;
        const FGuid CreatedId = People->CreatePerson(Person);
        if (CreatedId != ExpectedId)
        {
            OutFailureReason = FString::Printf(TEXT("%s:CreatePersonFailed"), Step);
            return false;
        }
        return true;
    };

    FPersonLite Player;
    Player.PersonId = OutIds.PlayerId;
    Player.DisplayName = TEXT("Lin Wei");
    Player.Gender = EPersonGender::Unspecified;
    Player.BirthRegion = OutIds.RegionId;
    Player.HomeRegion = OutIds.RegionId;
    Player.Occupation = TEXT("ShipMaintenanceTechnician");
    if (!CreatePerson(Player, TEXT("Player")))
    {
        return false;
    }

    FPersonLite Worker;
    Worker.PersonId = OutIds.WorkerId;
    Worker.DisplayName = TEXT("Mara Venn");
    Worker.Gender = EPersonGender::Female;
    Worker.BirthRegion = OutIds.RegionId;
    Worker.HomeRegion = OutIds.RegionId;
    Worker.Occupation = TEXT("DockSystemsOperator");
    if (!CreatePerson(Worker, TEXT("Worker")))
    {
        return false;
    }

    FPersonLite Traveller;
    Traveller.PersonId = OutIds.TravellerId;
    Traveller.DisplayName = TEXT("Tomas Rhee");
    Traveller.Gender = EPersonGender::Male;
    Traveller.BirthRegion = OutIds.RegionId;
    Traveller.HomeRegion = OutIds.RegionId;
    Traveller.Occupation = TEXT("Courier");
    if (!CreatePerson(Traveller, TEXT("Traveller")))
    {
        return false;
    }

    auto SetState = [Motivation, &OutFailureReason](FPersonCausalState State, const TCHAR* Step)
    {
        if (!Motivation->SetCausalState(State))
        {
            OutFailureReason = FString::Printf(TEXT("%s:SetCausalStateFailed"), Step);
            return false;
        }
        return true;
    };

    FPersonCausalState PlayerState;
    PlayerState.PersonId = OutIds.PlayerId;
    PlayerState.CurrentRegion = OutIds.RegionId;
    PlayerState.bAutonomous = false;
    PlayerState.Credits = 100.0f;
    PlayerState.Hunger = 0.35f;
    PlayerState.Fatigue = 0.20f;
    if (!SetState(PlayerState, TEXT("Player")))
    {
        return false;
    }

    FPersonCausalState WorkerState;
    WorkerState.PersonId = OutIds.WorkerId;
    WorkerState.CurrentRegion = OutIds.RegionId;
    WorkerState.bAutonomous = true;
    WorkerState.Credits = 45.0f;
    WorkerState.Hunger = 0.55f;
    WorkerState.Fatigue = 0.25f;
    WorkerState.IncomePressure = 0.70f;
    WorkerState.WorkObligation = 0.90f;
    if (!SetState(WorkerState, TEXT("Worker")))
    {
        return false;
    }

    FPersonCausalState TravellerState;
    TravellerState.PersonId = OutIds.TravellerId;
    TravellerState.CurrentRegion = OutIds.RegionId;
    TravellerState.bAutonomous = true;
    TravellerState.Credits = 250.0f;
    TravellerState.Hunger = 0.20f;
    TravellerState.Fatigue = 0.15f;
    TravellerState.Curiosity = 0.75f;
    TravellerState.RiskTolerance = 0.35f;
    if (!SetState(TravellerState, TEXT("Traveller")))
    {
        return false;
    }

    auto RegisterOpportunity = [Opportunities, &OutIds, &OutFailureReason](
        FWorldOpportunity Opportunity,
        const TCHAR* Step)
    {
        const FGuid ExpectedId = Opportunity.OpportunityId;
        const FGuid RegisteredId = Opportunities->RegisterOpportunity(Opportunity);
        if (RegisteredId != ExpectedId)
        {
            OutFailureReason = FString::Printf(TEXT("%s:RegisterOpportunityFailed"), Step);
            return false;
        }
        OutIds.OpportunityIds.Add(RegisteredId);
        return true;
    };

    auto MakeOpportunity = [&OutIds, Seed](
        const TCHAR* Label,
        const FName Type,
        const EWorldGoalType Goal,
        const float CreditReward,
        const int32 DurationMinutes,
        const int32 AvailableUses,
        const float Danger,
        const FString& Summary)
    {
        FWorldOpportunity Opportunity;
        Opportunity.OpportunityId = MakeStableGuid(Seed, Label);
        Opportunity.OpportunityType = Type;
        Opportunity.RegionId = OutIds.RegionId;
        Opportunity.AvailableFrom = FWorldTime(0);
        Opportunity.ExpiresAt = FWorldTime(1440);
        Opportunity.SupportedGoals.Add(Goal);
        Opportunity.Urgency = 0.5f;
        Opportunity.Danger = Danger;
        Opportunity.CreditReward = CreditReward;
        Opportunity.DurationMinutes = DurationMinutes;
        Opportunity.AvailableUses = AvailableUses;
        Opportunity.Summary = Summary;
        return Opportunity;
    };

    if (!RegisterOpportunity(
        MakeOpportunity(WorldSimDemoLabels::FoodOpportunity, TEXT("MealService"), EWorldGoalType::Eat,
            -15.0f, 30, -1, 0.0f, TEXT("Buy and eat a prepared meal")),
        TEXT("FoodOpportunity")))
    {
        return false;
    }
    if (!RegisterOpportunity(
        MakeOpportunity(WorldSimDemoLabels::WorkOpportunity, TEXT("DockWorkShift"), EWorldGoalType::Work,
            120.0f, 480, 1, 0.05f, TEXT("Complete a dock systems work shift")),
        TEXT("WorkOpportunity")))
    {
        return false;
    }
    if (!RegisterOpportunity(
        MakeOpportunity(WorldSimDemoLabels::RestOpportunity, TEXT("RestFacility"), EWorldGoalType::Rest,
            0.0f, 480, -1, 0.0f, TEXT("Rest in a licensed sleep pod")),
        TEXT("RestOpportunity")))
    {
        return false;
    }
    if (!RegisterOpportunity(
        MakeOpportunity(WorldSimDemoLabels::MedicalOpportunity, TEXT("MedicalCare"), EWorldGoalType::SeekMedicalCare,
            -80.0f, 120, -1, 0.0f, TEXT("Receive outpatient medical treatment")),
        TEXT("MedicalOpportunity")))
    {
        return false;
    }
    if (!RegisterOpportunity(
        MakeOpportunity(WorldSimDemoLabels::DangerOpportunity, TEXT("HazardInspection"), EWorldGoalType::Investigate,
            300.0f, 180, 1, 0.65f, TEXT("Inspect an unstable maintenance chamber")),
        TEXT("DangerOpportunity")))
    {
        return false;
    }

    OutIds.SafeCommitmentId = MakeStableGuid(Seed, WorldSimDemoLabels::SafeCommitment);
    FCommitment SafeCommitment;
    SafeCommitment.CommitmentId = OutIds.SafeCommitmentId;
    SafeCommitment.SubjectId = OutIds.TravellerId;
    SafeCommitment.CommitmentType = TEXT("ScheduledTransfer");
    SafeCommitment.OriginRegion = OutIds.RegionId;
    SafeCommitment.DestinationRegion = TEXT("Orbital_Transfer_Hub");
    SafeCommitment.PlannedStart = FWorldTime(120);
    SafeCommitment.PlannedEnd = FWorldTime(240);
    SafeCommitment.bHardCommitment = true;
    SafeCommitment.RiskLevel = 0.0f;
    if (Commitments->CreateCommitment(SafeCommitment) != OutIds.SafeCommitmentId)
    {
        OutFailureReason = TEXT("SafeCommitment:CreateCommitmentFailed");
        return false;
    }

    OutIds.RiskCommitmentId = MakeStableGuid(Seed, WorldSimDemoLabels::RiskCommitment);
    FCommitment RiskCommitment;
    RiskCommitment.CommitmentId = OutIds.RiskCommitmentId;
    RiskCommitment.SubjectId = OutIds.WorkerId;
    RiskCommitment.CommitmentType = TEXT("HazardResponseMission");
    RiskCommitment.OriginRegion = OutIds.RegionId;
    RiskCommitment.DestinationRegion = TEXT("Maintenance_Chamber_7");
    RiskCommitment.PlannedStart = FWorldTime(180);
    RiskCommitment.PlannedEnd = FWorldTime(360);
    RiskCommitment.bHardCommitment = true;
    RiskCommitment.RiskLevel = 0.65f;
    if (Commitments->CreateCommitment(RiskCommitment) != OutIds.RiskCommitmentId)
    {
        OutFailureReason = TEXT("RiskCommitment:CreateCommitmentFailed");
        return false;
    }

    bCreated = true;
    OutFailureReason.Reset();
    return true;
}
