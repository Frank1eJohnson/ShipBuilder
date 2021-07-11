// Spaceship Builder - Gwennaël Arbona

#include "NovaMainMenuFlight.h"

#include "NovaMainMenu.h"

#include "Game/NovaArea.h"
#include "Game/NovaGameMode.h"
#include "Game/NovaGameState.h"
#include "Game/NovaOrbitalSimulationComponent.h"

#include "Spacecraft/NovaSpacecraftPawn.h"
#include "Spacecraft/NovaSpacecraftMovementComponent.h"

#include "System/NovaMenuManager.h"
#include "Player/NovaPlayerController.h"

#include "UI/Widget/NovaButton.h"
#include "UI/Widget/NovaFadingWidget.h"
#include "UI/Widget/NovaKeyLabel.h"

#include "Nova.h"

#include "Widgets/Layout/SBackgroundBlur.h"

#define LOCTEXT_NAMESPACE "SNovaMainMenuFlight"

/*----------------------------------------------------
    HUD panel stack
----------------------------------------------------*/

DECLARE_DELEGATE_OneParam(FNovaHUDUpdate, int32);

class SNovaHUDPanel : public SNovaFadingWidget<false>
{
	SLATE_BEGIN_ARGS(SNovaHUDPanel)
	{}

	SLATE_ARGUMENT(FNovaHUDUpdate, OnUpdate)
	SLATE_DEFAULT_SLOT(FArguments, Content)

	SLATE_END_ARGS()

public:
	SNovaHUDPanel() : DesiredIndex(-1), CurrentIndex(-1)
	{}

	void Construct(const FArguments& InArgs)
	{
		// clang-format off
		SNovaFadingWidget::Construct(SNovaFadingWidget::FArguments().Content()
		[
			InArgs._Content.Widget
		]);
		// clang-format on

		UpdateCallback = InArgs._OnUpdate;
		SetColorAndOpacity(TAttribute<FLinearColor>(this, &SNovaFadingWidget::GetLinearColor));
	}

	void SetHUDIndex(int32 Index)
	{
		DesiredIndex = Index;
	}

protected:
	virtual bool IsDirty() const override
	{
		if (CurrentIndex != DesiredIndex)
		{
			return true;
		}

		return false;
	}

	virtual void OnUpdate() override
	{
		CurrentIndex = DesiredIndex;

		if (UpdateCallback.IsBound())
		{
			UpdateCallback.Execute(CurrentIndex);
		}
	}

protected:
	FNovaHUDUpdate UpdateCallback;
	int32          DesiredIndex;
	int32          CurrentIndex;
};

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

SNovaMainMenuFlight::SNovaMainMenuFlight()
	: PC(nullptr), SpacecraftPawn(nullptr), SpacecraftMovement(nullptr), GameState(nullptr), OrbitalSimulation(nullptr)
{}

void SNovaMainMenuFlight::Construct(const FArguments& InArgs)
{
	// Data
	const FNovaMainTheme& Theme = FNovaStyleSet::GetMainTheme();
	MenuManager                 = InArgs._MenuManager;

	// Parent constructor
	SNovaNavigationPanel::Construct(SNovaNavigationPanel::FArguments().Menu(InArgs._Menu));

	// Invisible widget
	SAssignNew(InvisibleWidget, SBox);

	// clang-format off
	ChildSlot
	.HAlign(HAlign_Center)
	.VAlign(VAlign_Bottom)
	[
		SNew(SVerticalBox)

		// HUD content
		+ SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Center)
		[
			SNew(SBackgroundBlur)
			.BlurRadius(this, &SNovaTabPanel::GetBlurRadius)
			.BlurStrength(this, &SNovaTabPanel::GetBlurStrength)
			.bApplyAlphaToBlur(true)
			.Padding(0)
			[
				SAssignNew(HUDPanel, SNovaHUDPanel)
				.OnUpdate(FNovaHUDUpdate::CreateSP(this, &SNovaMainMenuFlight::SetHUDIndexCallback))
				[
					SNew(SBorder)
					.BorderImage(&Theme.MainMenuBackground)
					.Padding(0)
					[
						SNew(SBorder)
						.BorderImage(&Theme.MainMenuGenericBackground)
						.Padding(0)
						[
							SAssignNew(HUDBox, SBox)
						]
					]
				]
			]
		]

		// HUD switcher
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SBackgroundBlur)
			.BlurRadius(this, &SNovaTabPanel::GetBlurRadius)
			.BlurStrength(this, &SNovaTabPanel::GetBlurStrength)
			.bApplyAlphaToBlur(true)
			.Padding(0)
			[
				SNew(SBorder)
				.BorderImage(&Theme.MainMenuBackground)
				.Padding(0)
				[
					SNew(SBorder)
					.BorderImage(&Theme.MainMenuGenericBorder)
					.Padding(FMargin(Theme.ContentPadding.Left, 0, Theme.ContentPadding.Right, 0))
					[
						SAssignNew(HUDSwitcher, SHorizontalBox)
					]
				]
			]
		]
	];

	// Layout our data
	FNovaHUDData& PowerHUD = HUDData[0];
	FNovaHUDData& AttitudeHUD = HUDData[1];
	FNovaHUDData& HomeHUD = HUDData[2];
	FNovaHUDData& OperationsHUD = HUDData[3];
	FNovaHUDData& WeaponsHUD = HUDData[4];
	
	/*----------------------------------------------------
	    Power
	----------------------------------------------------*/

	SAssignNew(PowerHUD.OverviewWidget, STextBlock)
		.TextStyle(&Theme.MainFont)
		.Text(LOCTEXT("Power", "Power"));

	SAssignNew(PowerHUD.DetailedWidget, SVerticalBox);

	PowerHUD.DefaultFocus = nullptr;
	
	/*----------------------------------------------------
	    Attitude
	----------------------------------------------------*/

	SAssignNew(AttitudeHUD.OverviewWidget, STextBlock)
		.TextStyle(&Theme.MainFont)
		.Text(LOCTEXT("Attitude", "Attitude"));

	SAssignNew(AttitudeHUD.DetailedWidget, SVerticalBox)
	
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNovaAssignNew(AlignManeuverButton, SNovaButton)
			.Action(FNovaPlayerInput::MenuPrimary)
			.Text(LOCTEXT("AlignManeuver", "Align to maneuver"))
			.HelpText(LOCTEXT("AlignManeuverHelp", "Allow thrusters to fire to re-orient the spacecraft"))
			.OnClicked(this, &SNovaMainMenuFlight::OnAlignToManeuver)
			.Enabled(this, &SNovaMainMenuFlight::IsManeuveringEnabled)
		];

	AttitudeHUD.DefaultFocus = AlignManeuverButton;
	
	/*----------------------------------------------------
	    Home
	----------------------------------------------------*/

	SAssignNew(HomeHUD.OverviewWidget, STextBlock)
		.TextStyle(&Theme.InfoFont)
		.Text(TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateLambda([=]()
		{
			return SpacecraftPawn ? SpacecraftPawn->GetSpacecraftCopy().GetName() : FText();
		})));

	/*----------------------------------------------------
	    Operations
	----------------------------------------------------*/

	SAssignNew(OperationsHUD.OverviewWidget, STextBlock)
		.TextStyle(&Theme.MainFont)
		.Text(LOCTEXT("Operations", "Operations"));

	SAssignNew(OperationsHUD.DetailedWidget, SVerticalBox)

		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNovaNew(SNovaButton)
			.Text(LOCTEXT("AbortFlightPlan", "Terminate flight plan"))
			.HelpText(LOCTEXT("AbortTrajectoryHelp", "Terminate the current flight plan and stay on the current orbit"))
			.OnClicked(FSimpleDelegate::CreateLambda([this]()
			{
				OrbitalSimulation->AbortTrajectory(GameState->GetPlayerSpacecraftIdentifiers());
			}))
			.Enabled(TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateLambda([&]()
			{
				return OrbitalSimulation && GameState && OrbitalSimulation->IsOnTrajectory(GameState->GetPlayerSpacecraftIdentifier());
			})))
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNovaAssignNew(FastForwardButton, SNovaButton)
			.Action(FNovaPlayerInput::MenuSecondary)
			.Text(LOCTEXT("FastForward", "Fast forward"))
			.HelpText(this, &SNovaMainMenuFlight::GetFastFowardHelp)
			.OnClicked(this, &SNovaMainMenuFlight::FastForward)
			.Enabled(this, &SNovaMainMenuFlight::CanFastForward)
		]
			
#if WITH_EDITOR && 0

		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNovaNew(SNovaButton)
			.Text(LOCTEXT("TimeDilation", "Disable time dilation"))
			.HelpText(LOCTEXT("TimeDilationHelp", "Set time dilation to zero"))
			.OnClicked(FSimpleDelegate::CreateLambda([this]()
			{
				GameState->SetTimeDilation(ENovaTimeDilation::Normal);
			}))
			.Enabled(TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateLambda([&]()
			{
				return GameState && GameState->CanDilateTime(ENovaTimeDilation::Normal);
			})))
		]
			
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNovaNew(SNovaButton)
			.Text(LOCTEXT("TimeDilation1", "Time dilation 1"))
			.HelpText(LOCTEXT("TimeDilation1Help", "Set time dilation to 1 (1s = 1m)"))
			.OnClicked(FSimpleDelegate::CreateLambda([this]()
			{
				GameState->SetTimeDilation(ENovaTimeDilation::Level1);
			}))
			.Enabled(TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateLambda([&]()
			{
				const ANovaGameState* GameState = MenuManager->GetWorld()->GetGameState<ANovaGameState>();
				return GameState && GameState->CanDilateTime(ENovaTimeDilation::Level1);
			})))
		]
			
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNovaNew(SNovaButton)
			.Text(LOCTEXT("TimeDilation2", "Time dilation 2"))
			.HelpText(LOCTEXT("TimeDilation2Help", "Set time dilation to 2 (1s = 20m)"))
			.OnClicked(FSimpleDelegate::CreateLambda([this]()
			{
				GameState->SetTimeDilation(ENovaTimeDilation::Level2);
			}))
			.Enabled(TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateLambda([&]()
			{
				return GameState && GameState->CanDilateTime(ENovaTimeDilation::Level2);
			})))
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNovaNew(SNovaButton)
			.Text(LOCTEXT("TestJoin", "Join random session"))
			.HelpText(LOCTEXT("TestJoinHelp", "Join random session"))
			.OnClicked(FSimpleDelegate::CreateLambda([&]()
			{
				PC->TestJoin();
			}))
		]
#endif // WITH_EDITOR
			
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNovaAssignNew(DockButton, SNovaButton)
			.Action(FNovaPlayerInput::MenuPrimary)
			.Text(this, &SNovaMainMenuFlight::GetDockingText)
			.HelpText(this, &SNovaMainMenuFlight::GetDockingHelp)
			.OnClicked(this, &SNovaMainMenuFlight::OnDockUndock)
			.Enabled(this, &SNovaMainMenuFlight::IsDockingEnabled)
		]
		
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNovaNew(SNovaButton)
			.Text(LOCTEXT("TestSilentRunning", "Silent running"))
			.OnClicked(FSimpleDelegate::CreateLambda([&]()
			{
				MenuManager->SetInterfaceColor(Theme.NegativeColor, FLinearColor(1.0f, 0.0f, 0.1f));
			}))
		];

	OperationsHUD.DefaultFocus = FastForwardButton;

	/*----------------------------------------------------
	    Weapons
	----------------------------------------------------*/

	SAssignNew(WeaponsHUD.OverviewWidget, STextBlock)
		.TextStyle(&Theme.MainFont)
		.Text(LOCTEXT("Weapons", "Weapons"));

	SAssignNew(WeaponsHUD.DetailedWidget, SVerticalBox);

	WeaponsHUD.DefaultFocus = nullptr;
	
	/*----------------------------------------------------
	    HUD panel construction
	----------------------------------------------------*/
	
	// Previous key
	HUDSwitcher->AddSlot()
	.AutoWidth()
	[
		SNew(SNovaKeyLabel)
		.Key(this, &SNovaMainMenuFlight::GetPreviousItemKey)
	];

	// Build HUD selection entries
	for (int32 HUDIndex = 0; HUDIndex < HUDCount; HUDIndex++)
	{
		HUDSwitcher->AddSlot()
		.AutoWidth()
		[
			SNew(SNovaButton) // No navigation
			.Focusable(false)
			.Theme("TabButton")
			.Size("HUDButtonSize")
			.HelpText(HUDData[HUDIndex].HelpText)
			.OnClicked(this, &SNovaMainMenuFlight::SetHUDIndex, HUDIndex)
			.UserSizeCallback(FNovaButtonUserSizeCallback::CreateLambda([=]
			{
				return HUDAnimation.GetAlpha(HUDIndex);
			}))
			.Enabled(TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateLambda([=]()
			{
				return HUDIndex != CurrentHUDIndex;
			})))
			.Content()
			[
				SNew(SBox)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					HUDData[HUDIndex].OverviewWidget.ToSharedRef()
				]
			]
		];
	}
	
	// Next key
	HUDSwitcher->AddSlot()
	.AutoWidth()
	[
		SNew(SNovaKeyLabel)
		.Key(this, &SNovaMainMenuFlight::GetNextItemKey)
	];

	// clang-format on

	// Initialize
	HUDAnimation.Initialize(FNovaStyleSet::GetButtonTheme().AnimationDuration);
}

/*----------------------------------------------------
    Interaction
----------------------------------------------------*/

void SNovaMainMenuFlight::Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime)
{
	SNovaTabPanel::Tick(AllottedGeometry, CurrentTime, DeltaTime);

	HUDAnimation.Update(CurrentHUDIndex, DeltaTime);
}

void SNovaMainMenuFlight::Show()
{
	SNovaTabPanel::Show();

	// Start the HUD at center
	SetHUDIndex(HUDCount / 2);
	HUDPanel->Reset();
	HUDAnimation.Update(CurrentHUDIndex, FNovaStyleSet::GetButtonTheme().AnimationDuration);
}

void SNovaMainMenuFlight::Hide()
{
	SNovaTabPanel::Hide();
}

void SNovaMainMenuFlight::UpdateGameObjects()
{
	PC                 = MenuManager.IsValid() ? MenuManager->GetPC() : nullptr;
	SpacecraftPawn     = IsValid(PC) ? PC->GetSpacecraftPawn() : nullptr;
	SpacecraftMovement = IsValid(SpacecraftPawn) ? SpacecraftPawn->GetSpacecraftMovement() : nullptr;
	GameState          = IsValid(PC) ? PC->GetWorld()->GetGameState<ANovaGameState>() : nullptr;
	OrbitalSimulation  = IsValid(GameState) ? GameState->GetOrbitalSimulation() : nullptr;
}

void SNovaMainMenuFlight::Next()
{
	SetHUDIndex(FMath::Min(CurrentHUDIndex + 1, HUDCount - 1));
}

void SNovaMainMenuFlight::Previous()
{
	SetHUDIndex(FMath::Max(CurrentHUDIndex - 1, 0));
}

void SNovaMainMenuFlight::HorizontalAnalogInput(float Value)
{
	if (IsValid(SpacecraftPawn))
	{
		SpacecraftPawn->PanInput(Value);
	}
}

void SNovaMainMenuFlight::VerticalAnalogInput(float Value)
{
	if (IsValid(SpacecraftPawn))
	{
		SpacecraftPawn->TiltInput(Value);
	}
}

TSharedPtr<SNovaButton> SNovaMainMenuFlight::GetDefaultFocusButton() const
{
	return HUDData[CurrentHUDIndex].DefaultFocus;
}

/*----------------------------------------------------
    Content callbacks
----------------------------------------------------*/

FKey SNovaMainMenuFlight::GetPreviousItemKey() const
{
	return MenuManager->GetFirstActionKey(FNovaPlayerInput::MenuPrevious);
}

FKey SNovaMainMenuFlight::GetNextItemKey() const
{
	return MenuManager->GetFirstActionKey(FNovaPlayerInput::MenuNext);
}

bool SNovaMainMenuFlight::CanFastForward() const
{
	const ANovaGameMode* GameMode = MenuManager->GetWorld()->GetAuthGameMode<ANovaGameMode>();
	return IsValid(GameMode) && GameMode->CanFastForward();
}

FText SNovaMainMenuFlight::GetFastFowardHelp() const
{
	FText HelpText = LOCTEXT("FastForwardHelp", "Wait until the next event");

	const ANovaGameMode* GameMode = MenuManager->GetWorld()->GetAuthGameMode<ANovaGameMode>();
	if (GameMode)
	{
		GameMode->CanFastForward(&HelpText);
	}

	return HelpText;
}

FText SNovaMainMenuFlight::GetDockingText() const
{
	if (IsValid(SpacecraftMovement) && SpacecraftMovement->GetState() == ENovaMovementState::Docked)
	{
		return LOCTEXT("Undock", "Undock");
	}
	else
	{
		return LOCTEXT("Dock", "Dock");
	}
}

FText SNovaMainMenuFlight::GetDockingHelp() const
{
	if (IsValid(SpacecraftMovement) && SpacecraftMovement->GetState() == ENovaMovementState::Docked)
	{
		return LOCTEXT("UndockHelp", "Undock from the station");
	}
	else
	{
		return LOCTEXT("DockHelp", "Dock at the station");
	}
}

bool SNovaMainMenuFlight::IsDockingEnabled() const
{
	if (OrbitalSimulation && IsValid(PC) && IsValid(SpacecraftMovement) && IsValid(SpacecraftPawn))
	{
		const FNovaTrajectory* PlayerTrajectory = OrbitalSimulation->GetPlayerTrajectory();
		return PlayerTrajectory == nullptr && SpacecraftMovement->CanDock() ||
			   !SpacecraftPawn->HasModifications() && SpacecraftPawn->IsSpacecraftValid() && SpacecraftMovement->CanUndock();
	}
	return false;
}

bool SNovaMainMenuFlight::IsManeuveringEnabled() const
{
	if (OrbitalSimulation)
	{
		const FNovaTrajectory* PlayerTrajectory = OrbitalSimulation->GetPlayerTrajectory();

		return PlayerTrajectory && IsValid(SpacecraftMovement) && SpacecraftMovement->GetState() == ENovaMovementState::Idle &&
			   !SpacecraftMovement->IsAlignedToManeuver();
	}

	return false;
}

/*----------------------------------------------------
    Callbacks
----------------------------------------------------*/

void SNovaMainMenuFlight::SetHUDIndex(int32 Index)
{
	NCHECK(Index >= 0 && Index < HUDCount);
	CurrentHUDIndex = Index;
	HUDPanel->SetHUDIndex(CurrentHUDIndex);
}

void SNovaMainMenuFlight::SetHUDIndexCallback(int32 Index)
{
	const FNovaMainTheme& Theme = FNovaStyleSet::GetMainTheme();

	// Apply the new widget
	TSharedPtr<SWidget>& NewWidget = HUDData[Index].DetailedWidget;
	if (NewWidget)
	{
		NewWidget->SetVisibility(EVisibility::Visible);
		HUDBox->SetContent(NewWidget.ToSharedRef());
		HUDBox->SetPadding(Theme.ContentPadding);
	}
	else
	{
		HUDBox->SetContent(InvisibleWidget.ToSharedRef());
		HUDBox->SetPadding(FMargin(0));
	}

	ResetNavigation();
}

void SNovaMainMenuFlight::FastForward()
{
	MenuManager->GetWorld()->GetAuthGameMode<ANovaGameMode>()->FastForward();
}

void SNovaMainMenuFlight::OnDockUndock()
{
	if (IsDockingEnabled())
	{
		if (IsValid(SpacecraftMovement) && SpacecraftMovement->GetState() == ENovaMovementState::Docked)
		{
			PC->Undock();
		}
		else
		{
			PC->Dock();
		}
	}
}

void SNovaMainMenuFlight::OnAlignToManeuver()
{
	if (IsValid(SpacecraftMovement))
	{
		SpacecraftMovement->AlignToManeuver();
	}
}

#undef LOCTEXT_NAMESPACE
