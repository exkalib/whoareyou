#include "TruthLedgerSubsystem.h"

FGuid UTruthLedgerSubsystem::RecordEvent(const FWorldEvent& Event)
{
    FWorldEvent StoredEvent = Event;
    if (!StoredEvent.EventId.IsValid())
    {
        StoredEvent.EventId = FGuid::NewGuid();
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
