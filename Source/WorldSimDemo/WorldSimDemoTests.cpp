#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Engine/World.h"
#include "MotivationSubsystem.h"
#include "OpportunityCompilerSubsystem.h"
#include "TruthLedgerSubsystem.h"
#include "UObject/Package.h"
#include "WorldSimDemoBootstrapSubsystem.h"
#include "WorldSimulationSubsystem.h"

namespace WorldSimDemoTests
{
class FScopedTestWorld
{
public:
    FScopedTestWorld()
    {
        static int32 NextWorldId = 1;
        const FName WorldName(*FString::Printf(TEXT("WorldSimAutomation_%d"), NextWorldId++));
        UPackage* WorldPackage = CreatePackage(
            *FString::Printf(TEXT("/Temp/%s"), *WorldName.ToString()));
        World = UWorld::CreateWorld(EWorldType::Game, false, WorldName, WorldPackage);
    }

    ~FScopedTestWorld()
    {
        if (World != nullptr)
        {
            World->DestroyWorld(false);
        }
    }

    UWorld* Get() const { return World; }

private:
    UWorld* World = nullptr;
};

FGuid TestGuid(const uint32 Value)
{
    return FGuid(Value, Value + 1, Value + 2, Value + 3);
}

bool EqualIds(const FDemoWorldIds& A, const FDemoWorldIds& B)
{
    return A.RegionId == B.RegionId
        && A.PlayerId == B.PlayerId
        && A.WorkerId == B.WorkerId
        && A.TravellerId == B.TravellerId
        && A.SafeCommitmentId == B.SafeCommitmentId
        && A.RiskCommitmentId == B.RiskCommitmentId
        && A.OpportunityIds == B.OpportunityIds;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorldSimTruthQueryTest, "WorldSim.M0.TruthQuery",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldSimTruthQueryTest::RunTest(const FString& Parameters)
{
    using namespace WorldSimDemoTests;
    FScopedTestWorld TestWorld;
    TestNotNull(TEXT("A temporary world is created"), TestWorld.Get());
    if (TestWorld.Get() == nullptr) return false;

    UTruthLedgerSubsystem* Ledger = TestWorld.Get()->GetSubsystem<UTruthLedgerSubsystem>();
    TestNotNull(TEXT("Truth ledger exists"), Ledger);
    if (Ledger == nullptr) return false;

    const FGuid SubjectA = TestGuid(100);
    const FGuid SubjectB = TestGuid(200);
    const FName RegionA(TEXT("RegionA"));
    const FName RegionB(TEXT("RegionB"));
    const int64 Minutes[] = {10, 20, 30, 40, 50, 60};
    const FGuid Subjects[] = {SubjectA, SubjectA, SubjectA, SubjectB, SubjectB, SubjectB};
    const FName Regions[] = {RegionA, RegionB, RegionA, RegionA, RegionB, RegionB};

    FWorldEvent FirstEvent;
    for (int32 Index = 0; Index < 6; ++Index)
    {
        FWorldEvent Event;
        Event.EventId = TestGuid(static_cast<uint32>(Index + 1));
        Event.OccurredAt = FWorldTime(Minutes[Index]);
        Event.EventType = Index % 2 == 0 ? FName(TEXT("Even")) : FName(TEXT("Odd"));
        Event.SubjectId = Subjects[Index];
        Event.RegionId = Regions[Index];
        Event.Summary = FString::Printf(TEXT("Event %d"), Index);
        TestEqual(TEXT("Each event is recorded with its supplied ID"), Ledger->RecordEvent(Event), Event.EventId);
        if (Index == 0) FirstEvent = Event;
    }

    FWorldEventQuery Query;
    Query.SubjectId = SubjectA;
    TestEqual(TEXT("Subject filter returns three events"), Ledger->QueryEvents(Query).Num(), 3);
    Query = FWorldEventQuery();
    Query.RegionId = RegionA;
    TestEqual(TEXT("Region filter returns three events"), Ledger->QueryEvents(Query).Num(), 3);
    Query = FWorldEventQuery();
    Query.bUseTimeRange = true;
    Query.FromInclusive = FWorldTime(20);
    Query.ToExclusive = FWorldTime(40);
    TestEqual(TEXT("Half-open time filter returns two events"), Ledger->QueryEvents(Query).Num(), 2);
    Query = FWorldEventQuery();
    Query.MaxResults = 1;
    TestEqual(TEXT("Maximum result limit is honored"), Ledger->QueryEvents(Query).Num(), 1);
    TestFalse(TEXT("A duplicate event ID is rejected"), Ledger->RecordEvent(FirstEvent).IsValid());
    TestEqual(TEXT("Duplicate rejection preserves event count"), Ledger->GetRecentEvents(100).Num(), 6);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorldSimStableBootstrapTest, "WorldSim.M0.StableBootstrap",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldSimStableBootstrapTest::RunTest(const FString& Parameters)
{
    using namespace WorldSimDemoTests;
    FScopedTestWorld FirstWorld;
    FScopedTestWorld SecondWorld;
    TestNotNull(TEXT("First temporary world is created"), FirstWorld.Get());
    TestNotNull(TEXT("Second temporary world is created"), SecondWorld.Get());
    if (FirstWorld.Get() == nullptr || SecondWorld.Get() == nullptr) return false;

    UWorldSimDemoBootstrapSubsystem* First = FirstWorld.Get()->GetSubsystem<UWorldSimDemoBootstrapSubsystem>();
    UWorldSimDemoBootstrapSubsystem* Second = SecondWorld.Get()->GetSubsystem<UWorldSimDemoBootstrapSubsystem>();
    TestNotNull(TEXT("First bootstrap exists"), First);
    TestNotNull(TEXT("Second bootstrap exists"), Second);
    if (First == nullptr || Second == nullptr) return false;

    FDemoWorldIds FirstIds;
    FDemoWorldIds SecondIds;
    FString Failure;
    TestTrue(TEXT("First demo world is created"), First->CreateDemoWorld(4242, FirstIds, Failure));
    TestTrue(TEXT("Second demo world is created"), Second->CreateDemoWorld(4242, SecondIds, Failure));
    TestTrue(TEXT("Equal seeds produce equal canonical IDs"), EqualIds(FirstIds, SecondIds));
    TestTrue(TEXT("All canonical person IDs are valid"),
        FirstIds.PlayerId.IsValid() && FirstIds.WorkerId.IsValid() && FirstIds.TravellerId.IsValid());
    TestEqual(TEXT("The demo exposes five stable opportunities"), FirstIds.OpportunityIds.Num(), 5);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorldSimQueryPurityTest, "WorldSim.M0.QueryPurity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldSimQueryPurityTest::RunTest(const FString& Parameters)
{
    using namespace WorldSimDemoTests;
    FScopedTestWorld TestWorld;
    TestNotNull(TEXT("A temporary world is created"), TestWorld.Get());
    if (TestWorld.Get() == nullptr) return false;

    UWorldSimDemoBootstrapSubsystem* Bootstrap = TestWorld.Get()->GetSubsystem<UWorldSimDemoBootstrapSubsystem>();
    UOpportunityCompilerSubsystem* Opportunities = TestWorld.Get()->GetSubsystem<UOpportunityCompilerSubsystem>();
    UMotivationSubsystem* Motivation = TestWorld.Get()->GetSubsystem<UMotivationSubsystem>();
    UWorldSimulationSubsystem* Simulation = TestWorld.Get()->GetSubsystem<UWorldSimulationSubsystem>();
    TestNotNull(TEXT("Bootstrap exists"), Bootstrap);
    TestNotNull(TEXT("Opportunity compiler exists"), Opportunities);
    TestNotNull(TEXT("Motivation subsystem exists"), Motivation);
    TestNotNull(TEXT("Simulation subsystem exists"), Simulation);
    if (Bootstrap == nullptr || Opportunities == nullptr || Motivation == nullptr || Simulation == nullptr) return false;

    FDemoWorldIds Ids;
    FString Failure;
    TestTrue(TEXT("Demo world is created"), Bootstrap->CreateDemoWorld(777, Ids, Failure));
    FDecisionTrace Trace;
    TestFalse(TEXT("No decision trace exists before a pure query"), Motivation->TryGetDecisionTrace(Ids.WorkerId, Trace));
    const TArray<FWorldOpportunity> Before = Opportunities->GetAvailableOpportunities(FWorldTime(0), Ids.RegionId, 100);
    const int32 ActivityCountBefore = Simulation->GetActiveActivities(100).Num();
    const TArray<FWorldOpportunity> After = Opportunities->GetAvailableOpportunities(FWorldTime(0), Ids.RegionId, 100);

    TestEqual(TEXT("Pure opportunity query preserves result count"), After.Num(), Before.Num());
    for (int32 Index = 0; Index < FMath::Min(Before.Num(), After.Num()); ++Index)
    {
        TestEqual(TEXT("Pure opportunity query preserves stable IDs"), After[Index].OpportunityId, Before[Index].OpportunityId);
        TestEqual(TEXT("Pure opportunity query does not consume uses"), After[Index].AvailableUses, Before[Index].AvailableUses);
    }
    TestFalse(TEXT("Pure query does not evaluate a decision"), Motivation->TryGetDecisionTrace(Ids.WorkerId, Trace));
    TestEqual(TEXT("Pure query does not create activities"), Simulation->GetActiveActivities(100).Num(), ActivityCountBefore);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorldSimBootstrapRepeatTest, "WorldSim.M0.BootstrapRepeat",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldSimBootstrapRepeatTest::RunTest(const FString& Parameters)
{
    using namespace WorldSimDemoTests;
    FScopedTestWorld TestWorld;
    TestNotNull(TEXT("A temporary world is created"), TestWorld.Get());
    if (TestWorld.Get() == nullptr) return false;

    UWorldSimDemoBootstrapSubsystem* Bootstrap = TestWorld.Get()->GetSubsystem<UWorldSimDemoBootstrapSubsystem>();
    TestNotNull(TEXT("Bootstrap exists"), Bootstrap);
    if (Bootstrap == nullptr) return false;

    FDemoWorldIds Ids;
    FString Failure;
    TestTrue(TEXT("Initial creation succeeds"), Bootstrap->CreateDemoWorld(99, Ids, Failure));
    TestTrue(TEXT("Bootstrap records successful creation"), Bootstrap->IsDemoWorldCreated());
    FDemoWorldIds RepeatedIds;
    TestFalse(TEXT("Repeated creation is rejected"), Bootstrap->CreateDemoWorld(99, RepeatedIds, Failure));
    TestEqual(TEXT("Repeated creation reports the canonical reason"), Failure, FString(TEXT("AlreadyCreated")));
    TestFalse(TEXT("Rejected creation returns no player ID"), RepeatedIds.PlayerId.IsValid());
    return true;
}

#endif
