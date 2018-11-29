// Nova project - Gwennaël Arbona

#include "NovaMainMenuInventory.h"

#include "NovaMainMenu.h"

#include "Nova/Game/NovaGameState.h"
#include "Nova/Spacecraft/NovaSpacecraftPawn.h"
#include "Nova/Spacecraft/System/NovaSpacecraftPropellantSystem.h"

#include "Nova/System/NovaMenuManager.h"
#include "Nova/Player/NovaPlayerController.h"

#include "Nova/UI/Widget/NovaFadingWidget.h"

#include "Nova/Nova.h"

#include "Widgets/Layout/SBackgroundBlur.h"
#include "Widgets/Layout/SScaleBox.h"

#define LOCTEXT_NAMESPACE "SNovaMainMenuInventory"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

void SNovaMainMenuInventory::Construct(const FArguments& InArgs)
{
	// Data
	const FNovaMainTheme& Theme = FNovaStyleSet::GetMainTheme();
	MenuManager                 = InArgs._MenuManager;

	// Parent constructor
	SNovaNavigationPanel::Construct(SNovaNavigationPanel::FArguments().Menu(InArgs._Menu));

	// Local data
	TSharedPtr<SVerticalBox> CargoBox;

	// clang-format off
	ChildSlot
	[
		SAssignNew(CargoBox, SVerticalBox)
	];

	// Cargo line generator
	auto BuildCargoLine = [&](FText Title)
	{
		TSharedPtr<SHorizontalBox> CargoLineBox;

		CargoBox->AddSlot()
		.AutoHeight()
		.Padding(Theme.ContentPadding)
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.TextStyle(&Theme.SubtitleFont)
				.Text(Title)
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SAssignNew(CargoLineBox, SHorizontalBox)
			]
		];

		for (int32 Index = 0; Index < ENovaConstants::MaxCompartmentCount; Index++)
		{
			CargoLineBox->AddSlot()
			.AutoWidth()
			[
				SNovaNew(SNovaButton)
				.Size("CompartmentButtonSize")
				//.HelpText()
				//.Enabled(this, &SNovaMainMenuAssembly::IsSelectCompartmentEnabled, Index)
				//.OnClicked(this, &SNovaMainMenuAssembly::OnCompartmentSelected, Index)
				.Content()
				[
					SNew(SOverlay)

					// Resource picture
					+ SOverlay::Slot()
					[
						SNew(SScaleBox)
						.Stretch(EStretch::ScaleToFill)
						[
							SNew(SNovaImage)
							.Image(FNovaImageGetter::CreateLambda([=]() -> const FSlateBrush*
							{
								if (IsValid(SpacecraftPawn) && Index >= 0 && Index < SpacecraftPawn->GetCompartmentCount())
								{
									const FNovaCompartment& Compartment    = SpacecraftPawn->GetCompartment(Index);
									if (IsValid(Compartment.Description))
									{
										return &Compartment.Description->AssetRender;
									}
								}

								return nullptr;
							}))
						]
					]

					// Compartment index
					+ SOverlay::Slot()
					.VAlign(VAlign_Top)
					[
						SNew(SBorder)
						.BorderImage(&Theme.MainMenuGenericBackground)
						.Padding(Theme.ContentPadding)
						[
							SNew(STextBlock)
							.Text(FText::AsNumber(Index + 1))
							.TextStyle(&Theme.MainFont)
						]
					]
				]
			];
		}
	};
	// clang-format on

	// BUild the procedural cargo lines
	BuildCargoLine(LOCTEXT("GeneralCargoTitle", "General cargo"));
	BuildCargoLine(LOCTEXT("BulkCargoTitle", "Bulk cargo"));
	BuildCargoLine(LOCTEXT("LiquidCargoTitle", "Liquid cargo"));
}

/*----------------------------------------------------
    Interaction
----------------------------------------------------*/

void SNovaMainMenuInventory::Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime)
{
	SNovaTabPanel::Tick(AllottedGeometry, CurrentTime, DeltaTime);
}

void SNovaMainMenuInventory::Show()
{
	SNovaTabPanel::Show();
}

void SNovaMainMenuInventory::Hide()
{
	SNovaTabPanel::Hide();
}

void SNovaMainMenuInventory::UpdateGameObjects()
{
	PC             = MenuManager.IsValid() ? MenuManager->GetPC() : nullptr;
	SpacecraftPawn = IsValid(PC) ? PC->GetSpacecraftPawn() : nullptr;
}

TSharedPtr<SNovaButton> SNovaMainMenuInventory::GetDefaultFocusButton() const
{
	return nullptr;
}

/*----------------------------------------------------
    Callbacks
----------------------------------------------------*/

#undef LOCTEXT_NAMESPACE