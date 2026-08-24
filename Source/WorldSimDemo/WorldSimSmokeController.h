#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WorldSimSmokeController.generated.h"

class UWorldSimSmokeWidget;

UCLASS(Blueprintable)
class WORLDSIMDEMO_API AWorldSimSmokeController : public AActor
{
    GENERATED_BODY()

public:
    AWorldSimSmokeController();
    virtual void Tick(float DeltaSeconds) override;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(EditDefaultsOnly, Category="World Simulation|Smoke")
    TSubclassOf<UWorldSimSmokeWidget> WidgetClass;

private:
    void TryCreateWidget();

    UPROPERTY(Transient)
    TObjectPtr<UWorldSimSmokeWidget> ActiveWidget;
};
