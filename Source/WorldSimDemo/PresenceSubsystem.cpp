#include "PresenceSubsystem.h"

bool UPresenceSubsystem::RegisterInterval(const FPresenceInterval& Interval)
{
    if (!Interval.PersonId.IsValid() || Interval.RegionId.IsNone() || Interval.End <= Interval.Start)
    {
        return false;
    }

    for (const FPresenceInterval& Existing : Intervals)
    {
        const bool bOverlaps = Interval.Start < Existing.End && Existing.Start < Interval.End;
        if (Existing.PersonId == Interval.PersonId && bOverlaps)
        {
            return false;
        }
    }
    Intervals.Add(Interval);
    return true;
}

bool UPresenceSubsystem::CanExistAt(const FGuid PersonId, const FName RegionId, const FWorldTime Time) const
{
    return GetStateAt(PersonId, RegionId, Time) != EWorldPresenceState::Unknown;
}

EWorldPresenceState UPresenceSubsystem::GetStateAt(const FGuid PersonId, const FName RegionId, const FWorldTime Time) const
{
    for (const FPresenceInterval& Interval : Intervals)
    {
        if (Interval.PersonId == PersonId && Interval.RegionId == RegionId && Interval.Contains(Time))
        {
            return Interval.State;
        }
    }
    return EWorldPresenceState::Unknown;
}
