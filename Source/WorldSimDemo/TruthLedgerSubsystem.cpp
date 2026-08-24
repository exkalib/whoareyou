#include "TruthLedgerSubsystem.h"

FGuid UTruthLedgerSubsystem::RecordEvent(const FWorldEvent& Event)
{
    if (Event.EventType.IsNone())
    {
        return FGuid();
    }

    FWorldEvent StoredEvent = Event;
    if (!StoredEvent.EventId.IsValid())
    {
        do
        {
            StoredEvent.EventId = FGuid::NewGuid();
        }
        while (Events.ContainsByPredicate([&StoredEvent](const FWorldEvent& Existing)
        {
            return Existing.EventId == StoredEvent.EventId;
        }));
    }
    else if (Events.ContainsByPredicate([&StoredEvent](const FWorldEvent& Existing)
    {
        return Existing.EventId == StoredEvent.EventId;
    }))
    {
        return FGuid();
    }

    Events.Add(StoredEvent);
    return StoredEvent.EventId;
}

TArray<FWorldEvent> UTruthLedgerSubsystem::GetEventsForSubject(const FGuid SubjectId) const
{
    TArray<FWorldEvent> Result;
    for (const FWorldEvent& Event : Events)
    {
        if (Event.SubjectId == SubjectId)
        {
            Result.Add(Event);
        }
    }
    return Result;
}

TArray<FWorldEvent> UTruthLedgerSubsystem::QueryEvents(const FWorldEventQuery& Query) const
{
    FWorldEventQuery NormalizedQuery = Query;
    NormalizedQuery.Normalize();
    if (!NormalizedQuery.IsValid())
    {
        return {};
    }

    TArray<FWorldEvent> Result;
    for (const FWorldEvent& Event : Events)
    {
        if (NormalizedQuery.SubjectId.IsValid() && Event.SubjectId != NormalizedQuery.SubjectId)
        {
            continue;
        }
        if (!NormalizedQuery.RegionId.IsNone() && Event.RegionId != NormalizedQuery.RegionId)
        {
            continue;
        }
        if (!NormalizedQuery.EventType.IsNone() && Event.EventType != NormalizedQuery.EventType)
        {
            continue;
        }
        if (NormalizedQuery.bUseTimeRange
            && (Event.OccurredAt < NormalizedQuery.FromInclusive
                || !(Event.OccurredAt < NormalizedQuery.ToExclusive)))
        {
            continue;
        }
        Result.Add(Event);
    }

    Result.Sort([&NormalizedQuery](const FWorldEvent& A, const FWorldEvent& B)
    {
        if (A.OccurredAt.Minute != B.OccurredAt.Minute)
        {
            return NormalizedQuery.bNewestFirst
                ? A.OccurredAt.Minute > B.OccurredAt.Minute
                : A.OccurredAt.Minute < B.OccurredAt.Minute;
        }

        const FString AId = A.EventId.ToString(EGuidFormats::Digits);
        const FString BId = B.EventId.ToString(EGuidFormats::Digits);
        return NormalizedQuery.bNewestFirst ? AId > BId : AId < BId;
    });

    if (Result.Num() > NormalizedQuery.MaxResults)
    {
        Result.SetNum(NormalizedQuery.MaxResults);
    }
    return Result;
}

TArray<FWorldEvent> UTruthLedgerSubsystem::GetRecentEvents(const int32 MaxResults) const
{
    FWorldEventQuery Query;
    Query.MaxResults = MaxResults;
    Query.bNewestFirst = true;
    return QueryEvents(Query);
}

bool UTruthLedgerSubsystem::TryGetEvent(const FGuid EventId, FWorldEvent& OutEvent) const
{
    if (!EventId.IsValid())
    {
        return false;
    }

    for (const FWorldEvent& Event : Events)
    {
        if (Event.EventId == EventId)
        {
            OutEvent = Event;
            return true;
        }
    }
    return false;
}
