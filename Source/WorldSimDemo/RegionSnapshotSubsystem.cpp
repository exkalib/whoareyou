#include "RegionSnapshotSubsystem.h"

FRegionSnapshot URegionSnapshotSubsystem::GetSnapshot(const FName RegionId) const
{
    if (const FRegionSnapshot* Snapshot = Snapshots.Find(RegionId))
    {
        return *Snapshot;
    }
    FRegionSnapshot EmptySnapshot;
    EmptySnapshot.RegionId = RegionId;
    return EmptySnapshot;
}

void URegionSnapshotSubsystem::SetSnapshot(const FRegionSnapshot& Snapshot)
{
    if (!Snapshot.RegionId.IsNone())
    {
        Snapshots.Add(Snapshot.RegionId, Snapshot);
    }
}
