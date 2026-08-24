#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WorldSimSmokeWidget.generated.h"

class UTextBlock;

UCLASS(Blueprintable)
class WORLDSIMDEMO_API UWorldSimSmokeWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual void NativeConstruct() override;

private:
    UFUNCTION()
    void HandleInitialize();

    UFUNCTION()
    void HandleAdvance10();

    UFUNCTION()
    void HandleAdvance60();

    UFUNCTION()
    void HandleAdvance1440();

    UFUNCTION()
    void HandleRefresh();

    void BuildInterface();
    void AdvanceAndRefresh(int32 Minutes);
    void RefreshSnapshot();

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> StatusText;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> WorldText;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> PeopleText;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> EventsText;
};
