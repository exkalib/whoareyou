#include "WorldSimSmokeController.h"

#include "WorldSimSmokeWidget.h"

AWorldSimSmokeController::AWorldSimSmokeController()
{
    PrimaryActorTick.bCanEverTick = false;
    WidgetClass = UWorldSimSmokeWidget::StaticClass();
}

void AWorldSimSmokeController::BeginPlay()
{
    Super::BeginPlay();
    if (GetNetMode() == NM_DedicatedServer || WidgetClass == nullptr)
    {
        return;
    }
    ActiveWidget = CreateWidget<UWorldSimSmokeWidget>(GetWorld(), WidgetClass);
    if (ActiveWidget != nullptr)
    {
        ActiveWidget->AddToViewport(100);
    }
}
