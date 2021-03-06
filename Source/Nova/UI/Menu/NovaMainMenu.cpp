// Nova project - Gwennaël Arbona

#include "NovaMainMenu.h"

#include "NovaMainMenuHome.h"
#include "NovaMainMenuGame.h"
#include "NovaMainMenuFlight.h"
#include "NovaMainMenuNavigation.h"
#include "NovaMainMenuInventory.h"
#include "NovaMainMenuAssembly.h"
#include "NovaMainMenuSettings.h"

#include "Nova/UI/Widget/NovaButton.h"
#include "Nova/UI/Widget/NovaFadingWidget.h"
#include "Nova/UI/Widget/NovaKeyLabel.h"
#include "Nova/UI/Widget/NovaTabView.h"
#include "Nova/UI/Widget/NovaWindowManipulator.h"
#include "Nova/UI/NovaUITypes.h"

#include "Nova/Spacecraft/NovaSpacecraftPawn.h"
#include "Nova/Spacecraft/NovaSpacecraftMovementComponent.h"

#include "Nova/Game/NovaGameState.h"
#include "Nova/Player/NovaPlayerController.h"
#include "Nova/System/NovaMenuManager.h"

#include "Nova/Nova.h"

#define LOCTEXT_NAMESPACE "SNovaMainMenu"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

void SNovaMainMenu::Construct(const FArguments& InArgs)
{
	// Data
	const FNovaMainTheme& Theme = FNovaStyleSet::GetMainTheme();
	InArgs._MenuManager->SetInterfaceColor(FLinearColor::White, Theme.PositiveColor);

	// Parent constructor
	SNovaMenu::Construct(SNovaMenu::FArguments().MenuManager(InArgs._MenuManager));

	// clang-format off
	MainContainer->SetContent(
		SAssignNew(TabView, SNovaTabView)

		// Navigate left
		.LeftNavigation()
		[
			SNew(SNovaKeyLabel)
			.Key(this, &SNovaMainMenu::GetPreviousTabKey)
		]

		// Navigate right
		.RightNavigation()
		[
			SNew(SNovaKeyLabel)
			.Key(this, &SNovaMainMenu::GetNextTabKey)
		]

		// Close button
		.End()
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNovaNew(SNovaButton)
				.Theme("TabButton")
				.Size("SmallButtonSize")
				.Icon(FNovaStyleSet::GetBrush("Icon/SB_Maximize"))
				.HelpText(LOCTEXT("MaximizeRestore", "Maximize or restore the window"))
				.OnClicked(this, &SNovaMainMenu::OnMaximizeRestore)
				.Visibility(this, &SNovaMainMenu::GetMaximizeVisibility)
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNovaNew(SNovaButton)
				.Theme("TabButton")
				.Size("TabButtonSize")
				.Action(FNovaPlayerInput::MenuToggle)
				.Text(this, &SNovaMainMenu::GetCloseText)
				.HelpText(this, &SNovaMainMenu::GetCloseHelpText)
				.OnClicked(this, &SNovaMainMenu::OnClose)
			]
		]

		// Header widget
		.Header()
		[
			SNew(SBorder)
			.Padding(Theme.ContentPadding)
			.BorderImage(&Theme.MainMenuGenericBorder)
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Left)
				[
					SAssignNew(Tooltip, SNovaText)
					.TextStyle(&Theme.MainFont)
					.Text(FNovaTextGetter::CreateSP(this, &SNovaMainMenu::GetTooltipText))
				]

				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.TextStyle(&Theme.MainFont)
					.Text(this, &SNovaMainMenu::GetDateText)
					.Visibility(this, &SNovaMainMenu::GetInfoTextVisibility)
				]
		
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Right)
				[
					SNew(SNovaText)
					.TextStyle(&Theme.MainFont)
					.Text(FNovaTextGetter::CreateSP(this, &SNovaMainMenu::GetInfoText))
					.Visibility(this, &SNovaMainMenu::GetInfoTextVisibility)
				]
			]
		]

		// Manipulator
		.BackgroundWidget()
		[
			SNew(SNovaMenuManipulator)
			.Image(&Theme.MainMenuManipulator)
			.ColorAndOpacity(this, &SNovaMainMenu::GetManipulatorColor)
		]

		// Home menu
		+ SNovaTabView::Slot()
		.Header(LOCTEXT("HomeMenuTitle", "Home"))
		.HeaderHelp(LOCTEXT("MainMenuTitleHelp", "Start playing"))
		.Visible(TAttribute<bool>::FGetter::CreateSP(this, &SNovaMainMenu::IsHomeMenuVisible))
		.Blur()
		[
			SAssignNew(HomeMenu, SNovaMainMenuHome)
			.Menu(this)
			.MenuManager(MenuManager)
		]

		// Game menu
		+ SNovaTabView::Slot()
		.Header(LOCTEXT("GameMenuTitle", "Game"))
		.HeaderHelp(LOCTEXT("GameMenuTitleHelp", "Play with your friends"))
		//.Visible(TAttribute<bool>::FGetter::CreateSP(this, &SNovaMainMenu::AreGameMenusVisible))
		.Visible(TAttribute<bool>::FGetter::CreateLambda([](){return false;}))
		.Blur()
		[
			SAssignNew(GameMenu, SNovaMainMenuGame)
			.Menu(this)
			.MenuManager(MenuManager)
		]

		// Flight menu
		+ SNovaTabView::Slot()
		.Header(LOCTEXT("FlightMenuTitle", "Flight"))
		.HeaderHelp(LOCTEXT("FlightMenuTitleHelp", "Control your ship"))
		.Visible(TAttribute<bool>::FGetter::CreateSP(this, &SNovaMainMenu::AreGameMenusVisible))
		[
			SAssignNew(FlightMenu, SNovaMainMenuFlight)
			.Menu(this)
			.MenuManager(MenuManager)
		]

		// Navigation menu
		+ SNovaTabView::Slot()
		.Header(LOCTEXT("NavigationMenuTitle", "Navigation"))
		.HeaderHelp(LOCTEXT("NavigationMenuTitleHelp", "Plot trajectories"))
		.Visible(TAttribute<bool>::FGetter::CreateSP(this, &SNovaMainMenu::AreGameMenusVisible))
		.Blur()
		[
			SAssignNew(NavigationMenu, SNovaMainMenuNavigation)
			.Menu(this)
			.MenuManager(MenuManager)
		]

		// Inventory menu
		+ SNovaTabView::Slot()
		.Header(LOCTEXT("InventoryMenuTitle", "Inventory"))
		.HeaderHelp(LOCTEXT("InventoryMenuTitleHelp", "Manage your spacecraft's fuel and cargo"))
		.Visible(TAttribute<bool>::FGetter::CreateSP(this, &SNovaMainMenu::AreGameMenusVisible))
		.Blur()
		[
			SAssignNew(InventoryMenu, SNovaMainMenuInventory)
			.Menu(this)
			.MenuManager(MenuManager)
		]

		// Assembly menu
		+ SNovaTabView::Slot()
		.Header(LOCTEXT("AssemblyMenuTitle", "Assembly"))
		.HeaderHelp(LOCTEXT("AssemblyMenuTitleHelp", "Edit spacecraft assembly"))
		.Visible(TAttribute<bool>::FGetter::CreateSP(this, &SNovaMainMenu::IsAssemblyMenuVisible))
		[
			SAssignNew(AssemblyMenu, SNovaMainMenuAssembly)
			.Menu(this)
			.MenuManager(MenuManager)
		]

		// Settings
		+ SNovaTabView::Slot()
		.Header(LOCTEXT("SettingsMenuTitle", "Settings"))
		.HeaderHelp(LOCTEXT("SettingsMenuTitleHelp", "Setup the game"))
		.Blur()
		[
			SAssignNew(SettingsMenu, SNovaMainMenuSettings)
			.Menu(this)
			.MenuManager(MenuManager)
		]
	);
	// clang-format on

	// Create ourselves a modal panel
	ModalPanel = CreateModalPanel();

	// Register menus
	MenuManager->RegisterGameMenu(HomeMenu);
	MenuManager->RegisterGameMenu(GameMenu);
	MenuManager->RegisterGameMenu(FlightMenu);
	MenuManager->RegisterGameMenu(NavigationMenu);
	MenuManager->RegisterGameMenu(InventoryMenu);
	MenuManager->RegisterGameMenu(AssemblyMenu);
	MenuManager->RegisterGameMenu(SettingsMenu);

	// Set the default menu
	WasOnMainMenu = true;
	TabView->SetTabIndex(static_cast<uint8>(ENovaMainMenuType::Home));
}

/*----------------------------------------------------
    Interaction
----------------------------------------------------*/

void SNovaMainMenu::Show()
{
	// Set the default menu
	bool IsOnMenu = MenuManager->GetPC()->IsOnMainMenu();
	if (IsOnMenu != WasOnMainMenu)
	{
		ENovaMainMenuType DesiredMenu = IsOnMenu ? ENovaMainMenuType::Home : ENovaMainMenuType::Flight;

		const ANovaSpacecraftPawn* SpacecraftPawn = MenuManager->GetPC()->GetSpacecraftPawn();
		if (IsValid(SpacecraftPawn) && SpacecraftPawn->GetCompartmentCount() == 0)
		{
			DesiredMenu = ENovaMainMenuType::Assembly;
		}

		TabView->SetTabIndex(static_cast<uint8>(DesiredMenu));
		WasOnMainMenu = IsOnMenu;
	}

	SetVisibility(EVisibility::Visible);
}

void SNovaMainMenu::Hide()
{
	SetVisibility(EVisibility::Hidden);
}

void SNovaMainMenu::ShowTooltip(SWidget* TargetWidget, FText Content)
{
	if (TargetWidget != CurrentTooltipWidget)
	{
		CurrentTooltipWidget = TargetWidget;
		CurrentTooltipText   = Content;
	}
}

void SNovaMainMenu::HideTooltip(SWidget* TargetWidget)
{
	if (TargetWidget == CurrentTooltipWidget)
	{
		CurrentTooltipWidget = nullptr;
		CurrentTooltipText   = FText();
	}

	// If the current focus is valid, and different, show it again
	TSharedPtr<SNovaButton> FocusButton = GetFocusedButton();
	if (FocusButton.IsValid() && FocusButton.Get() != TargetWidget)
	{
		ShowTooltip(FocusButton.Get(), FocusButton->GetHelpText());
	}
}

FReply SNovaMainMenu::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& KeyEvent)
{
	FReply Result = SNovaMenu::OnKeyDown(MyGeometry, KeyEvent);

	if (!Result.IsEventHandled())
	{
		const FKey Key = KeyEvent.GetKey();

		// Move between tabs
		if (CurrentNavigationPanel && !CurrentNavigationPanel->IsModal())
		{
			if (IsActionKey(FNovaPlayerInput::MenuPreviousTab, Key))
			{
				TabView->ShowPreviousTab();
				Result = FReply::Handled();
			}
			else if (IsActionKey(FNovaPlayerInput::MenuNextTab, Key))
			{
				TabView->ShowNextTab();
				Result = FReply::Handled();
			}
		}
	}

	return Result;
}

/*----------------------------------------------------
    Content callbacks
----------------------------------------------------*/

bool SNovaMainMenu::IsHomeMenuVisible() const
{
	return MenuManager.IsValid() && IsValid(MenuManager->GetPC()) && MenuManager->GetPC()->IsOnMainMenu();
}

bool SNovaMainMenu::IsAssemblyMenuVisible() const
{
	if (AreGameMenusVisible() && IsValid(MenuManager->GetPC()))
	{
		const ANovaSpacecraftPawn* SpacecraftPawn = MenuManager->GetPC()->GetSpacecraftPawn();
		if (IsValid(SpacecraftPawn))
		{
			return SpacecraftPawn->IsDocked();
		}
	}

	return false;
}

bool SNovaMainMenu::AreGameMenusVisible() const
{
	return !IsHomeMenuVisible();
}

EVisibility SNovaMainMenu::GetInfoTextVisibility() const
{
	return AreGameMenusVisible() ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SNovaMainMenu::GetMaximizeVisibility() const
{
	return GEngine->GetGameUserSettings()->GetFullscreenMode() == EWindowMode::Windowed ? EVisibility::Visible : EVisibility::Collapsed;
}

FText SNovaMainMenu::GetCloseText() const
{
	ANovaPlayerController* PC = MenuManager->GetPC();

	if (PC && PC->IsOnMainMenu())
	{
		return LOCTEXT("QuitGame", "Quit game");
	}
	else if (PC && PC->IsMenuOnly())
	{
		return LOCTEXT("QuitMenu", "Quit to menu");
	}
	else
	{
		return LOCTEXT("Close", "Close menu");
	}
}

FText SNovaMainMenu::GetCloseHelpText() const
{
	ANovaPlayerController* PC = MenuManager->GetPC();

	if (PC && PC->IsOnMainMenu())
	{
		return LOCTEXT("QuitGameHelp", "Exit the game");
	}
	else if (PC && PC->IsMenuOnly())
	{
		return LOCTEXT("QuitMenuHelp", "Save and log out of the current game");
	}
	else
	{
		return LOCTEXT("CloseHelp", "Close the main menu");
	}
}

FText SNovaMainMenu::GetTooltipText() const
{
	return CurrentTooltipText;
}

FText SNovaMainMenu::GetDateText() const
{
	const ANovaGameState* GameState = MenuManager->GetWorld()->GetGameState<ANovaGameState>();
	NCHECK(GameState);

	return ::GetDateText(GameState->GetCurrentTime());
}

FText SNovaMainMenu::GetInfoText() const
{
	FNovaCredits PlayerCredits = MenuManager->GetPC()->GetAccountBalance();

	return FText::FormatNamed(LOCTEXT("InfoText", "{credits} in account"), TEXT("credits"), GetPriceText(PlayerCredits));
}

FSlateColor SNovaMainMenu::GetManipulatorColor() const
{
	return MenuManager->GetInterfaceColor();
}

FKey SNovaMainMenu::GetPreviousTabKey() const
{
	return MenuManager->GetFirstActionKey(FNovaPlayerInput::MenuPreviousTab);
}

FKey SNovaMainMenu::GetNextTabKey() const
{
	return MenuManager->GetFirstActionKey(FNovaPlayerInput::MenuNextTab);
}

/*----------------------------------------------------
    Action callbacks
----------------------------------------------------*/

void SNovaMainMenu::OnMaximizeRestore()
{
	MenuManager->MaximizeOrRestore();
}

void SNovaMainMenu::OnClose()
{
	if (MenuManager->GetPC()->IsOnMainMenu())
	{
		MenuManager->GetPC()->ExitGame();
	}
	else if (MenuManager->GetPC()->IsMenuOnly())
	{
		const ANovaSpacecraftPawn* SpacecraftPawn = MenuManager->GetPC()->GetSpacecraftPawn();

		// Ship is docked, we will save and quit
		if (IsValid(SpacecraftPawn) && SpacecraftPawn->IsDocked())
		{
			ModalPanel->Show(LOCTEXT("ConfirmQuit", "Quit to menu ?"), LOCTEXT("ConfirmQuitHelp", "Your progression will be saved."),
				FSimpleDelegate::CreateLambda(
					[&]()
					{
						MenuManager->GetPC()->GoToMainMenu(true);
					}));
		}

		// Ship is not docked, some progress will be lost
		else
		{
			const UNovaGameInstance* GameInstance = MenuManager->GetWorld()->GetGameInstance<UNovaGameInstance>();
			NCHECK(GameInstance);
			double MinutesSinceLastSave = GameInstance->GetMinutesSinceLastSave();

			ModalPanel->Show(LOCTEXT("ConfirmQuitWithoutSaving", "Quit without saving ?"),
				FText::FormatNamed(LOCTEXT("ConfirmQuitWithoutSavingHelp",
									   "You will lose progress since the last save, {minutes} {minutes}|plural(one=minute,other=minutes) "
									   "ago. Dock at a station to save the game."),
					TEXT("minutes"), FText::AsNumber(FMath::CeilToInt(MinutesSinceLastSave))),
				FSimpleDelegate::CreateLambda(
					[&]()
					{
						MenuManager->GetPC()->GoToMainMenu(false);
					}));
		}
	}
	else
	{
		MenuManager->GetPC()->ToggleMenuOrQuit();
	}
}

#undef LOCTEXT_NAMESPACE
