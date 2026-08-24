#include "LocalPersonManagerSubsystem.h"
#include "PresenceSubsystem.h"
#include "PersonSubsystem.h"
#include "DailyRoutineSubsystem.h"

TArray<FGuid> ULocalPersonManagerSubsystem::RefreshVisiblePeople(const FName RegionId, const FWorldTime Time, const int32 MaxVisiblePeople)
{
    ActiveRegion = RegionId;
    const int32 Limit = FMath::Max(0, MaxVisiblePeople);
    TArray<FGuid> Visible;

    if (const UWorld* World = GetWorld())
    {
        const UPersonSubsystem* People = World->GetSubsystem<UPersonSubsystem>();
        const UDailyRoutineSubsystem* Routines = World->GetSubsystem<UDailyRoutineSubsystem>();
        if (People == nullptr || Routines == nullptr)
        {
            return Visible;
        }

        for (const FPersonLite& Person : People->GetPeopleInRegion(RegionId))
        {
            if (Visible.Num() >= Limit)
            {
                break;
            }
            if (!IsPersonAvailable(Person.PersonId, RegionId, Time))
            {
                continue;
            }

            FDailyScheduleEntry Activity;
            if (!Routines->TryGetActivityAt(Person.PersonId, Time, Activity))
            {
                continue;
            }

            FPersonFull Full;
            if (InstantiatePerson(Person.PersonId, RegionId, Time, Full))
            {
                Full.CurrentActivitySummary = UEnum::GetValueAsString(Activity.Activity);
                ActivePeople.Add(Person.PersonId, Full);
                Visible.Add(Person.PersonId);
            }
        }
    }
    return Visible;
}

bool ULocalPersonManagerSubsystem::InstantiatePerson(const FGuid PersonId, const FName RegionId, const FWorldTime Time, FPersonFull& OutPerson)
{
    if (!IsPersonAvailable(PersonId, RegionId, Time))
    {
        return false;
    }

    if (const FPersonFull* Existing = ActivePeople.Find(PersonId))
    {
        OutPerson = *Existing;
        return true;
    }

    if (const UWorld* World = GetWorld())
    {
        if (const UPersonSubsystem* People = World->GetSubsystem<UPersonSubsystem>())
        {
            FPersonLite Lite;
            if (People->TryGetPerson(PersonId, Lite))
            {
                FPersonFull Full;
                Full.Lite = Lite;
                Full.AppearanceSeed = PersonId.ToString(EGuidFormats::Digits);
                Full.bIsInstantiated = true;
                ActivePeople.Add(PersonId, Full);
                OutPerson = Full;
                return true;
            }
        }
    }
    return false;
}

void ULocalPersonManagerSubsystem::RecyclePerson(const FGuid PersonId)
{
    ActivePeople.Remove(PersonId);
}

TArray<FGuid> ULocalPersonManagerSubsystem::GetInstantiatedPeople() const
{
    TArray<FGuid> Result;
    ActivePeople.GetKeys(Result);
    return Result;
}

bool ULocalPersonManagerSubsystem::IsPersonAvailable(const FGuid PersonId, const FName RegionId, const FWorldTime Time) const
{
    if (const UWorld* World = GetWorld())
    {
        if (const UPresenceSubsystem* Presence = World->GetSubsystem<UPresenceSubsystem>())
        {
            return Presence->CanExistAt(PersonId, RegionId, Time);
        }
    }
    return false;
}

void ULocalPersonManagerSubsystem::SetRegion(const FName RegionId)
{
    ActiveRegion = RegionId;
}
