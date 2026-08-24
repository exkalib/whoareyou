#include "PersonSubsystem.h"

FGuid UPersonSubsystem::CreatePerson(FPersonLite Person)
{
    if (!Person.PersonId.IsValid())
    {
        Person.PersonId = FGuid::NewGuid();
    }
    if (Person.DisplayName.IsEmpty() || Person.HomeRegion.IsNone())
    {
        return FGuid();
    }
    People.Add(Person.PersonId, Person);
    return Person.PersonId;
}

bool UPersonSubsystem::TryGetPerson(const FGuid PersonId, FPersonLite& OutPerson) const
{
    if (const FPersonLite* Person = People.Find(PersonId))
    {
        OutPerson = *Person;
        return true;
    }
    return false;
}

TArray<FPersonLite> UPersonSubsystem::GetPeopleInRegion(const FName RegionId) const
{
    TArray<FPersonLite> Result;
    for (const TPair<FGuid, FPersonLite>& Pair : People)
    {
        if (Pair.Value.HomeRegion == RegionId)
        {
            Result.Add(Pair.Value);
        }
    }
    return Result;
}

TArray<FPersonLite> UPersonSubsystem::GetPeople(const int32 MaxResults) const
{
    TArray<FPersonLite> Result;
    People.GenerateValueArray(Result);
    Result.Sort([](const FPersonLite& A, const FPersonLite& B)
    {
        return A.PersonId.ToString(EGuidFormats::Digits)
            < B.PersonId.ToString(EGuidFormats::Digits);
    });

    const int32 BoundedMaxResults = FMath::Clamp(MaxResults, 1, 1000);
    if (Result.Num() > BoundedMaxResults)
    {
        Result.SetNum(BoundedMaxResults);
    }
    return Result;
}
