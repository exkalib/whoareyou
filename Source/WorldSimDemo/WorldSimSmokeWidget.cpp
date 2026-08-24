#include "WorldSimSmokeWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "WorldSimDebugSubsystem.h"
#include "WorldSimDemoBootstrapSubsystem.h"
#include "WorldSimulationSubsystem.h"

namespace WorldSimSmokeStyle
{
    const FLinearColor Background(0.018f, 0.035f, 0.055f, 0.96f);
    const FLinearColor Panel(0.035f, 0.075f, 0.095f, 1.0f);
    const FLinearColor Accent(0.95f, 0.46f, 0.12f, 1.0f);
    const FLinearColor Text(0.86f, 0.93f, 0.91f, 1.0f);
    const FLinearColor Muted(0.52f, 0.68f, 0.67f, 1.0f);
}

void UWorldSimSmokeWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (WidgetTree != nullptr && WidgetTree->RootWidget == nullptr)
    {
        BuildInterface();
    }
    RefreshSnapshot();
}

void UWorldSimSmokeWidget::BuildInterface()
{
    UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Backdrop"));
    Backdrop->SetBrushColor(WorldSimSmokeStyle::Background);
    Backdrop->SetPadding(FMargin(32.0f));
    WidgetTree->RootWidget = Backdrop;

    UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Root"));
    Backdrop->SetContent(Root);

    auto AddText = [this, Root](const TCHAR* Name, const FString& Value, const int32 Size, const FLinearColor Color)
    {
        UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName(Name));
        Text->SetText(FText::FromString(Value));
        Text->SetColorAndOpacity(FSlateColor(Color));
        FSlateFontInfo Font = Text->GetFont();
        Font.Size = Size;
        Text->SetFont(Font);
        Text->SetAutoWrapText(true);
        UVerticalBoxSlot* Slot = Root->AddChildToVerticalBox(Text);
        Slot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
        return Text;
    };

    AddText(TEXT("Title"), TEXT("PORT ASTER / CAUSAL WORLD OBSERVATORY"), 28, WorldSimSmokeStyle::Accent);
    StatusText = AddText(TEXT("Status"), TEXT("Demo world is not initialized."), 15, WorldSimSmokeStyle::Muted);
    WorldText = AddText(TEXT("World"), TEXT("WORLD MINUTE 0"), 20, WorldSimSmokeStyle::Text);

    UHorizontalBox* Controls = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Controls"));
    UVerticalBoxSlot* ControlSlot = Root->AddChildToVerticalBox(Controls);
    ControlSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 16.0f));

    auto AddButton = [this, Controls](const TCHAR* Name, const TCHAR* Label, const FName HandlerName)
    {
        UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), FName(Name));
        Button->SetBackgroundColor(WorldSimSmokeStyle::Panel);
        UTextBlock* LabelText = WidgetTree->ConstructWidget<UTextBlock>();
        LabelText->SetText(FText::FromString(Label));
        LabelText->SetColorAndOpacity(FSlateColor(WorldSimSmokeStyle::Text));
        Button->SetContent(LabelText);
        FScriptDelegate ClickDelegate;
        ClickDelegate.BindUFunction(this, HandlerName);
        Button->OnClicked.Add(ClickDelegate);
        UHorizontalBoxSlot* Slot = Controls->AddChildToHorizontalBox(Button);
        Slot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
    };

    AddButton(TEXT("Initialize"), TEXT("INITIALIZE"), GET_FUNCTION_NAME_CHECKED(UWorldSimSmokeWidget, HandleInitialize));
    AddButton(TEXT("Advance10"), TEXT("+10 MIN"), GET_FUNCTION_NAME_CHECKED(UWorldSimSmokeWidget, HandleAdvance10));
    AddButton(TEXT("Advance60"), TEXT("+60 MIN"), GET_FUNCTION_NAME_CHECKED(UWorldSimSmokeWidget, HandleAdvance60));
    AddButton(TEXT("Advance1440"), TEXT("+1 DAY"), GET_FUNCTION_NAME_CHECKED(UWorldSimSmokeWidget, HandleAdvance1440));
    AddButton(TEXT("Refresh"), TEXT("REFRESH"), GET_FUNCTION_NAME_CHECKED(UWorldSimSmokeWidget, HandleRefresh));

    UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("DataScroll"));
    UVerticalBoxSlot* ScrollSlot = Root->AddChildToVerticalBox(Scroll);
    ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    UVerticalBox* Data = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Data"));
    Scroll->AddChild(Data);
    PeopleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("People"));
    PeopleText->SetColorAndOpacity(FSlateColor(WorldSimSmokeStyle::Text));
    PeopleText->SetAutoWrapText(true);
    Data->AddChildToVerticalBox(PeopleText)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 18.0f));
    EventsText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Events"));
    EventsText->SetColorAndOpacity(FSlateColor(WorldSimSmokeStyle::Muted));
    EventsText->SetAutoWrapText(true);
    Data->AddChildToVerticalBox(EventsText);
}

void UWorldSimSmokeWidget::HandleInitialize()
{
    UWorld* World = GetWorld();
    UWorldSimDemoBootstrapSubsystem* Bootstrap = World != nullptr
        ? World->GetSubsystem<UWorldSimDemoBootstrapSubsystem>() : nullptr;
    if (Bootstrap == nullptr)
    {
        StatusText->SetText(FText::FromString(TEXT("ERROR / Bootstrap subsystem unavailable.")));
        return;
    }

    FDemoWorldIds Ids;
    FString Failure;
    const bool bCreated = Bootstrap->CreateDemoWorld(4242, Ids, Failure);
    StatusText->SetText(FText::FromString(bCreated
        ? TEXT("ONLINE / Demo world initialized with deterministic seed 4242.")
        : FString::Printf(TEXT("REJECTED / %s"), *Failure)));
    RefreshSnapshot();
}

void UWorldSimSmokeWidget::HandleAdvance10() { AdvanceAndRefresh(10); }
void UWorldSimSmokeWidget::HandleAdvance60() { AdvanceAndRefresh(60); }
void UWorldSimSmokeWidget::HandleAdvance1440() { AdvanceAndRefresh(1440); }
void UWorldSimSmokeWidget::HandleRefresh() { RefreshSnapshot(); }

void UWorldSimSmokeWidget::AdvanceAndRefresh(const int32 Minutes)
{
    UWorld* World = GetWorld();
    UWorldSimulationSubsystem* Simulation = World != nullptr
        ? World->GetSubsystem<UWorldSimulationSubsystem>() : nullptr;
    if (Simulation == nullptr)
    {
        StatusText->SetText(FText::FromString(TEXT("ERROR / Simulation subsystem unavailable.")));
        return;
    }
    Simulation->AdvanceSimulationMinutes(Minutes);
    StatusText->SetText(FText::FromString(FString::Printf(TEXT("ADVANCED / %d world minutes."), Minutes)));
    RefreshSnapshot();
}

void UWorldSimSmokeWidget::RefreshSnapshot()
{
    if (WorldText == nullptr || PeopleText == nullptr || EventsText == nullptr)
    {
        return;
    }
    UWorld* World = GetWorld();
    const UWorldSimDebugSubsystem* Debug = World != nullptr
        ? World->GetSubsystem<UWorldSimDebugSubsystem>() : nullptr;
    if (Debug == nullptr)
    {
        return;
    }

    FSimulationDebugLimits Limits;
    const FWorldDebugSnapshot Snapshot = Debug->BuildSnapshot(Limits);
    WorldText->SetText(FText::FromString(FString::Printf(
        TEXT("WORLD MINUTE %lld  /  PEOPLE %d  /  ACTIVE %d  /  COMMITMENTS %d"),
        Snapshot.WorldTime.Minute, Snapshot.People.Num(), Snapshot.Activities.Num(), Snapshot.Commitments.Num())));

    FString People(TEXT("PEOPLE\n"));
    for (const FPersonDebugSnapshot& Person : Snapshot.People)
    {
        const FPersonCausalState& State = Person.CausalState;
        const FString Goal = StaticEnum<EWorldGoalType>()->GetNameStringByValue(static_cast<int64>(Person.ChosenGoal));
        FString Activity(TEXT("Idle"));
        for (const FActiveWorldActivity& Active : Snapshot.Activities)
        {
            if (Active.PersonId == Person.Person.PersonId)
            {
                Activity = FString::Printf(TEXT("%s (%d min left)"), *Active.Title, Active.RemainingMinutes);
                break;
            }
        }
        People += FString::Printf(
            TEXT("\n%s / %s\nHunger %.0f%%  Fatigue %.0f%%  Health %.0f%%  Credits %.0f\nGoal %s  /  Activity %s\nReason %s\n"),
            *Person.Person.DisplayName, *Person.Person.Occupation.ToString(),
            State.Hunger * 100.0f, State.Fatigue * 100.0f, State.Health * 100.0f, State.Credits,
            *Goal, *Activity, *Person.ChosenReason);
    }
    PeopleText->SetText(FText::FromString(People));

    FString Events(TEXT("RECENT FACTS\n"));
    for (const FWorldEvent& Event : Snapshot.RecentEvents)
    {
        Events += FString::Printf(TEXT("[%lld] %s / %s\n"),
            Event.OccurredAt.Minute, *Event.EventType.ToString(), *Event.Summary);
    }
    EventsText->SetText(FText::FromString(Events));
}
