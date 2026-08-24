#include "DailyRoutineSubsystem.h"

void UDailyRoutineSubsystem::GenerateDefaultDay(const FPersonLite& Person, const FName WorkRegion)
{
    if (!Person.PersonId.IsValid())
    {
        return;
    }

    TArray<FDailyScheduleEntry> Day;
    auto AddEntry = [&Day, &Person](const EDailyActivity Activity, const FName Region, const int64 StartMinute, const int64 EndMinute)
    {
        FDailyScheduleEntry Entry;
        Entry.PersonId = Person.PersonId;
        Entry.Activity = Activity;
        Entry.RegionId = Region;
        Entry.Start = FWorldTime(StartMinute);
        Entry.End = FWorldTime(EndMinute);
        Day.Add(Entry);
    };

    AddEntry(EDailyActivity::Sleep, Person.HomeRegion, 0, 390);
    AddEntry(EDailyActivity::Eat, Person.HomeRegion, 390, 450);
    AddEntry(EDailyActivity::Commute, Person.HomeRegion, 450, 510);
    AddEntry(EDailyActivity::Work, WorkRegion, 510, 1050);
    AddEntry(EDailyActivity::Commute, WorkRegion, 1050, 1110);
    AddEntry(EDailyActivity::Eat, Person.HomeRegion, 1110, 1170);
    AddEntry(EDailyActivity::Leisure, Person.HomeRegion, 1170, 1320);
    AddEntry(EDailyActivity::Sleep, Person.HomeRegion, 1320, 1440);
    FDailySchedule Schedule;
    Schedule.Entries = MoveTemp(Day);
    Schedules.Add(Person.PersonId, MoveTemp(Schedule));
}

TArray<FDailyScheduleEntry> UDailyRoutineSubsystem::GetSchedule(const FGuid PersonId) const
{
    if (const FDailySchedule* Schedule = Schedules.Find(PersonId))
    {
        return Schedule->Entries;
    }
    return TArray<FDailyScheduleEntry>();
}

bool UDailyRoutineSubsystem::TryGetActivityAt(const FGuid PersonId, const FWorldTime Time, FDailyScheduleEntry& OutEntry) const
{
    if (const FDailySchedule* Schedule = Schedules.Find(PersonId))
    {
        for (const FDailyScheduleEntry& Entry : Schedule->Entries)
        {
            if (Entry.Contains(Time))
            {
                OutEntry = Entry;
                return true;
            }
        }
    }
    return false;
}
