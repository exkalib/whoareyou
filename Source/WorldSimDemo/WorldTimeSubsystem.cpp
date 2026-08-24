#include "WorldTimeSubsystem.h"

void UWorldTimeSubsystem::AdvanceMinutes(const int32 Minutes)
{
    CurrentTime = CurrentTime + FMath::Max(0, Minutes);
}
