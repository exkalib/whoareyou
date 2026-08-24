#include "WorldSimDemoGameMode.h"

#include "EngineUtils.h"
#include "WorldSimSmokeController.h"

void AWorldSimDemoGameMode::BeginPlay()
{
    Super::BeginPlay();
    for (TActorIterator<AWorldSimSmokeController> It(GetWorld()); It; ++It)
    {
        return;
    }
    GetWorld()->SpawnActor<AWorldSimSmokeController>();
}
