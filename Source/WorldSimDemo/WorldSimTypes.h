#pragma once

#include "CoreMinimal.h"
#include "WorldSimTypes.generated.h"

UENUM(BlueprintType)
enum class EWorldPresenceState : uint8
{
    Unknown,
    Present,
    InTransit,
    Absent,
    Deceased
};

UENUM(BlueprintType)
enum class ECommitmentState : uint8
{
    Planned,
    Confirmed,
    Departed,
    InTransit,
    Completed,
    Cancelled,
    Delayed,
    Failed
};

UENUM(BlueprintType)
enum class ECommitmentOutcome : uint8
{
    None,
    Succeeded,
    Failed,
    Injured,
    Missing,
    Deceased
};

UENUM(BlueprintType)
enum class EPersonLifeState : uint8
{
    Active,
    Missing,
    Deceased
};

UENUM(BlueprintType)
enum class EPersonGender : uint8
{
    Unspecified,
    Female,
    Male,
    NonBinary
};

UENUM(BlueprintType)
enum class EDailyActivity : uint8
{
    Sleep,
    Eat,
    Commute,
    Work,
    Leisure,
    Travel
};

UENUM(BlueprintType)
enum class EWorldGoalType : uint8
{
    None,
    Eat,
    Rest,
    Work,
    SeekMedicalCare,
    Socialize,
    Investigate,
    Train,
    StaySafe
};

USTRUCT(BlueprintType)
struct WORLDSIMDEMO_API FWorldTime
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int64 Minute = 0;

    FWorldTime() = default;
    explicit FWorldTime(const int64 InMinute) : Minute(InMinute) {}

    FWorldTime operator+(const int64 DeltaMinutes) const { return FWorldTime(Minute + DeltaMinutes); }
    bool operator<(const FWorldTime& Other) const { return Minute < Other.Minute; }
    bool operator<=(const FWorldTime& Other) const { return Minute <= Other.Minute; }
};

USTRUCT(BlueprintType)
struct WORLDSIMDEMO_API FPresenceInterval
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGuid PersonId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName RegionId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FWorldTime Start;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FWorldTime End;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EWorldPresenceState State = EWorldPresenceState::Unknown;

    bool Contains(const FWorldTime& Time) const { return Start <= Time && Time < End; }
};

USTRUCT(BlueprintType)
struct WORLDSIMDEMO_API FCommitment
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGuid CommitmentId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGuid SubjectId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName CommitmentType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName OriginRegion;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName DestinationRegion;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FWorldTime PlannedStart;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FWorldTime PlannedEnd;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ECommitmentState State = ECommitmentState::Planned;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bHardCommitment = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0", ClampMax="1.0"))
    float RiskLevel = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    ECommitmentOutcome Outcome = ECommitmentOutcome::None;
};

USTRUCT(BlueprintType)
struct WORLDSIMDEMO_API FPersonLite
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGuid PersonId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EPersonGender Gender = EPersonGender::Unspecified;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName BirthRegion;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName HomeRegion;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName Occupation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGuid HouseholdId;
};

USTRUCT(BlueprintType)
struct WORLDSIMDEMO_API FPersonFull
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FPersonLite Lite;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString AppearanceSeed;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString CurrentActivitySummary;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bIsInstantiated = false;
};

USTRUCT(BlueprintType)
struct WORLDSIMDEMO_API FPersonCausalState
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bAutonomous = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EPersonLifeState LifeState = EPersonLifeState::Active;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGuid PersonId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName CurrentRegion;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0", ClampMax="1.0"))
    float Hunger = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0", ClampMax="1.0"))
    float Fatigue = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0", ClampMax="1.0"))
    float Health = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0", ClampMax="1.0"))
    float Stress = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0", ClampMax="1.0"))
    float Loneliness = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0", ClampMax="1.0"))
    float IncomePressure = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0", ClampMax="1.0"))
    float WorkObligation = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0", ClampMax="1.0"))
    float FamilyPressure = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0", ClampMax="1.0"))
    float Curiosity = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0", ClampMax="1.0"))
    float RiskTolerance = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0", ClampMax="1.0"))
    float TrainingNeed = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Credits = 0.0f;
};

USTRUCT(BlueprintType)
struct WORLDSIMDEMO_API FDecisionCandidate
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EWorldGoalType Goal = EWorldGoalType::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Utility = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Reason;
};

USTRUCT(BlueprintType)
struct WORLDSIMDEMO_API FDecisionTrace
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGuid PersonId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FWorldTime EvaluatedAt;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EWorldGoalType ChosenGoal = EWorldGoalType::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString ChosenReason;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FDecisionCandidate> Candidates;
};

USTRUCT(BlueprintType)
struct WORLDSIMDEMO_API FWorldOpportunity
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGuid OpportunityId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName OpportunityType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGuid ProviderId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName RegionId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FWorldTime AvailableFrom;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FWorldTime ExpiresAt;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<EWorldGoalType> SupportedGoals;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0", ClampMax="1.0"))
    float Urgency = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0", ClampMax="1.0"))
    float Danger = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float CreditReward = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1"))
    int32 DurationMinutes = 60;

    // -1 means unlimited uses until the opportunity expires.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="-1"))
    int32 AvailableUses = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Summary;

    bool IsAvailableAt(const FWorldTime& Time) const
    {
        return AvailableFrom <= Time && Time < ExpiresAt;
    }
};

USTRUCT(BlueprintType)
struct WORLDSIMDEMO_API FDailyScheduleEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGuid PersonId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EDailyActivity Activity = EDailyActivity::Leisure;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName RegionId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FWorldTime Start;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FWorldTime End;

    bool Contains(const FWorldTime& Time) const { return Start <= Time && Time < End; }
};

USTRUCT(BlueprintType)
struct WORLDSIMDEMO_API FWorldEvent
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGuid EventId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FWorldTime OccurredAt;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName EventType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGuid SubjectId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName RegionId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Summary;
};

USTRUCT(BlueprintType)
struct WORLDSIMDEMO_API FWorldMessage
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGuid MessageId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGuid RelatedEventId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGuid RecipientId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName SourceType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FWorldTime ReceivedAt;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Confidence = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Content;
};

USTRUCT(BlueprintType)
struct WORLDSIMDEMO_API FMessageKnowledge
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGuid KnowerId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGuid MessageId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FWorldTime LearnedAt;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0", ClampMax="1.0"))
    float BeliefConfidence = 0.5f;
};

USTRUCT(BlueprintType)
struct WORLDSIMDEMO_API FRegionSnapshot
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName RegionId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FWorldTime AtTime;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 VisiblePopulation = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 ActiveJobs = 0;
};
USTRUCT(BlueprintType)
struct WORLDSIMDEMO_API FActiveWorldActivity
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGuid PersonId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGuid OpportunityId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Title;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName RegionId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EWorldGoalType Goal = EWorldGoalType::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 RemainingMinutes = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float CreditReward = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0", ClampMax="1.0"))
    float Danger = 0.0f;
};

USTRUCT(BlueprintType)
struct WORLDSIMDEMO_API FWorldEventQuery
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGuid SubjectId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName RegionId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName EventType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FWorldTime FromInclusive;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FWorldTime ToExclusive;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bUseTimeRange = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1", ClampMax="1000"))
    int32 MaxResults = 100;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bNewestFirst = true;

    void Normalize()
    {
        MaxResults = FMath::Clamp(MaxResults, 1, 1000);
    }

    bool IsValid() const
    {
        return MaxResults > 0
            && MaxResults <= 1000
            && (!bUseTimeRange || FromInclusive < ToExclusive);
    }
};

USTRUCT(BlueprintType)
struct WORLDSIMDEMO_API FSimulationDebugLimits
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1", ClampMax="1000"))
    int32 MaxPeople = 100;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1", ClampMax="1000"))
    int32 MaxActivities = 100;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1", ClampMax="1000"))
    int32 MaxCommitments = 100;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1", ClampMax="1000"))
    int32 MaxEvents = 200;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1", ClampMax="1000"))
    int32 MaxMessages = 200;

    void Normalize()
    {
        MaxPeople = FMath::Clamp(MaxPeople, 1, 1000);
        MaxActivities = FMath::Clamp(MaxActivities, 1, 1000);
        MaxCommitments = FMath::Clamp(MaxCommitments, 1, 1000);
        MaxEvents = FMath::Clamp(MaxEvents, 1, 1000);
        MaxMessages = FMath::Clamp(MaxMessages, 1, 1000);
    }

    bool IsValid() const
    {
        return MaxPeople > 0 && MaxPeople <= 1000
            && MaxActivities > 0 && MaxActivities <= 1000
            && MaxCommitments > 0 && MaxCommitments <= 1000
            && MaxEvents > 0 && MaxEvents <= 1000
            && MaxMessages > 0 && MaxMessages <= 1000;
    }
};

USTRUCT(BlueprintType)
struct WORLDSIMDEMO_API FDemoWorldIds
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    FName RegionId;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    FGuid PlayerId;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    FGuid WorkerId;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    FGuid TravellerId;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    FGuid SafeCommitmentId;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    FGuid RiskCommitmentId;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TArray<FGuid> OpportunityIds;
};
