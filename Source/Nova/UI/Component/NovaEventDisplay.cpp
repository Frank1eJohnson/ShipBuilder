// Nova project - Gwennaël Arbona

#include "NovaEventDisplay.h"

#include "Nova/Game/NovaArea.h"
#include "Nova/Game/NovaGameModeStates.h"
#include "Nova/Game/NovaGameState.h"
#include "Nova/Game/NovaOrbitalSimulationComponent.h"

#include "Nova/Player/NovaPlayerController.h"
#include "Nova/Spacecraft/NovaSpacecraftPawn.h"
#include "Nova/Spacecraft/NovaSpacecraftMovementComponent.h"

#include "Nova/System/NovaAssetManager.h"
#include "Nova/System/NovaGameInstance.h"
#include "Nova/System/NovaMenuManager.h"

#include "Nova/Nova.h"

#include "Widgets/Layout/SBackgroundBlur.h"

#define LOCTEXT_NAMESPACE "NovaEventDisplay"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

SNovaEventDisplay::SNovaEventDisplay()
{}

void SNovaEventDisplay::Construct(const FArguments& InArgs)
{
	const FNovaMainTheme& Theme = FNovaStyleSet::GetMainTheme();

	// Settings
	MenuManager = InArgs._MenuManager;

	// clang-format off
	SNovaFadingWidget::Construct(SNovaFadingWidget::FArguments()
		.ColorAndOpacity(this, &SNovaEventDisplay::GetDisplayColor));

	ChildSlot
	.HAlign(HAlign_Center)
	.VAlign(VAlign_Top)
	.Padding(FMargin(0, 90))
	[
		SNew(SBorder)
		.BorderImage(&Theme.MainMenuGenericBorder)
		.ColorAndOpacity(this, &SNovaFadingWidget::GetLinearColor)
		.BorderBackgroundColor(this, &SNovaFadingWidget::GetSlateColor)
		.Padding(2)
		.Visibility(this, &SNovaEventDisplay::GetMainVisibility)
		[
			SNew(SBackgroundBlur)
			.BlurRadius(Theme.BlurRadius)
			.BlurStrength(Theme.BlurStrength)
			.bApplyAlphaToBlur(true)
			.Padding(0)
			[
				SNew(SBox)
				.WidthOverride(Theme.EventDisplayWidth)
				[
					SNew(SBorder)
					.HAlign(HAlign_Center)
					.BorderImage(&Theme.MainMenuGenericBackground)
					.Padding(Theme.ContentPadding)
					[
						SNew(SVerticalBox)

						+ SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Center)
						[
							SNew(STextBlock)
							.TextStyle(&Theme.HeadingFont)
							.Text(this, &SNovaEventDisplay::GetMainText)
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Center)
						[
							SNew(STextBlock)
							.TextStyle(&Theme.MainFont)
							.Text(this, &SNovaEventDisplay::GetTimeText)
							.Visibility(this, &SNovaEventDisplay::GetDetailsVisibility)
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Center)
						[
							SNew(SHorizontalBox)

							+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							[
								SNew(SNovaImage)
								.Image(FNovaImageGetter::CreateSP(this, &SNovaEventDisplay::GetIcon))
								.Visibility(this, &SNovaEventDisplay::GetDetailsVisibility)
							]

							+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							[
								SNew(SNovaText)
								.TextStyle(&Theme.MainFont)
								.Text(FNovaTextGetter::CreateSP(this, &SNovaEventDisplay::GetDetailsText))
								.Visibility(this, &SNovaEventDisplay::GetDetailsVisibility)
							]
						]
					]
				]
			]
		]
	];
	// clang-format on
}

/*----------------------------------------------------
    Interface
----------------------------------------------------*/

void SNovaEventDisplay::Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime)
{
	SNovaFadingWidget::Tick(AllottedGeometry, CurrentTime, DeltaTime);

	DesiredState = FNovaEventDisplayData();
	TimeText     = FText();
	DetailsText  = FText();

	if (MenuManager.IsValid())
	{
		auto PC                 = MenuManager->GetPC();
		auto SpacecraftPawn     = IsValid(PC) ? PC->GetSpacecraftPawn() : nullptr;
		auto SpacecraftMovement = IsValid(SpacecraftPawn) ? SpacecraftPawn->GetSpacecraftMovement() : nullptr;
		auto GameState          = MenuManager->GetWorld()->GetGameState<ANovaGameState>();
		auto OrbitalSimulation  = IsValid(GameState) ? GameState->GetOrbitalSimulation() : nullptr;

		if (IsValid(PC) && IsValid(SpacecraftPawn) && IsValid(GameState) && IsValid(OrbitalSimulation) && !SpacecraftPawn->IsDocked() &&
			(PC->GetCameraState() == ENovaPlayerCameraState::Default || PC->GetCameraState() == ENovaPlayerCameraState::Chase))
		{
			const FNovaTrajectory* Trajectory      = OrbitalSimulation->GetPlayerTrajectory();
			const FNovaTime&       CurrentGameTime = GameState->GetCurrentTime();

			// Trajectory
			if (Trajectory)
			{
				FNovaTime ManeuverTimeLeft = Trajectory->GetNextManeuverStartTime(CurrentGameTime) - CurrentGameTime -
											 FNovaTime::FromSeconds(CommonCutsceneDelay - 1);
				ManeuverTimeLeft = FMath::Max(ManeuverTimeLeft, FNovaTime(0));

				// Nearing maneuver
				if (ManeuverTimeLeft < FNovaTime::FromSeconds(60))
				{
					DesiredState.Text       = LOCTEXT("ImminentManeuver", "Imminent maneuver").ToUpper();
					DesiredState.HasDetails = true;

					if (CurrentState.HasDetails)
					{
						TimeText = ManeuverTimeLeft >= FNovaTime::FromSeconds(1)
									 ? FText::FormatNamed(LOCTEXT("ImminentManeuverTimeFormat", "{time} left"), TEXT("time"),
										   GetDurationText(ManeuverTimeLeft))
									 : FText();

						DetailsText    = SpacecraftMovement->IsMainDriveEnabled()
										   ? LOCTEXT("ImminentManeuverClear", "Spacecraft is cleared to maneuver")
										   : LOCTEXT("ImminentManeuverNotClear", "Spacecraft is not cleared to maneuver");
						IsValidDetails = SpacecraftMovement->IsMainDriveEnabled();
					}
				}

				// On trajectory
				else
				{
					DesiredState.Text = LOCTEXT("OnTrajectory", "On trajectory");
				}
			}

			// Free flight
			else
			{
				if (GameState->GetCurrentArea()->Hidden)
				{
					DesiredState.Text = LOCTEXT("orbit", "On orbit");
				}
				else
				{
					DesiredState.Text = FText::FormatNamed(
						LOCTEXT("FreeFlightFormat", "On orbit at {station}"), TEXT("station"), GameState->GetCurrentArea()->Name);
				}
			}
		}
	}
}

/*----------------------------------------------------
    Content callbacks
----------------------------------------------------*/

EVisibility SNovaEventDisplay::GetMainVisibility() const
{
	return CurrentState.Text.IsEmpty() ? EVisibility::Hidden : EVisibility::Visible;
}

EVisibility SNovaEventDisplay::GetDetailsVisibility() const
{
	return DetailsText.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible;
}

FLinearColor SNovaEventDisplay::GetDisplayColor() const
{
	return 1.25f * MenuManager->GetHighlightColor();
}

FText SNovaEventDisplay::GetMainText() const
{
	return CurrentState.Text;
}

FText SNovaEventDisplay::GetTimeText() const
{
	return TimeText;
}

FText SNovaEventDisplay::GetDetailsText() const
{
	return DetailsText;
}

const FSlateBrush* SNovaEventDisplay::GetIcon() const
{
	return IsValidDetails ? FNovaStyleSet::GetBrush("Icon/SB_On") : FNovaStyleSet::GetBrush("Icon/SB_Off");
}

#undef LOCTEXT_NAMESPACE
