#include "WorldSimSmokeController.h"

#include "WorldSimSmokeWidget.h"

AWorldSimSmokeController::AWorldSimSmokeController()
{
    PrimaryActorTick.bCanEverTick = true;
    WidgetClass = UWorldSimSmokeWidget::StaticClass();
}

void AWorldSimSmokeController::BeginPlay()
{
    Super::BeginPlay();
    TryCreateWidget();
}

void AWorldSimSmokeController::Tick(const float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    TryCreateWidget();
}

void AWorldSimSmokeController::TryCreateWidget()
{
    if (ActiveWidget != nullptr)
    {
        SetActorTickEnabled(false);
        return;
    }
    if (GetNetMode() == NM_DedicatedServer || WidgetClass == nullptr)
    {
        SetActorTickEnabled(false);
        return;
    }

    APlayerController* PlayerController = GetWorld() != nullptr
        ? GetWorld()->GetFirstPlayerController() : nullptr;
    if (PlayerController == nullptr || !PlayerController->IsLocalController())
    {
        return;
    }

    ActiveWidget = CreateWidget<UWorldSimSmokeWidget>(PlayerController, WidgetClass);
    if (ActiveWidget != nullptr)
    {
        ActiveWidget->AddToViewport(100);
        SetActorTickEnabled(false);
        UE_LOG(LogTemp, Display, TEXT("WorldSim smoke interface added to the local player viewport."));
    }
}
