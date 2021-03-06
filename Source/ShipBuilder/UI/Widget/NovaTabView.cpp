// Spaceship Builder - Gwennaël Arbona

#include "NovaTabView.h"
#include "NovaMenu.h"

#include "UI/NovaUITypes.h"
#include "Nova.h"

#include "Engine/Engine.h"

#include "Widgets/Layout/SBackgroundBlur.h"

/*----------------------------------------------------
    Tab view content widget
----------------------------------------------------*/

SNovaTabPanel::SNovaTabPanel() : SNovaNavigationPanel(), Blurred(false), TabIndex(0), CurrentVisible(false), CurrentAlpha(0)
{}

void SNovaTabPanel::Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime)
{
	SNovaNavigationPanel::Tick(AllottedGeometry, CurrentTime, DeltaTime);

	// Update opacity value
	if (TabIndex != ParentTabView->GetDesiredTabIndex())
	{
		CurrentAlpha -= (DeltaTime / ENovaUIConstants::FadeDurationShort);
	}
	else if (TabIndex == ParentTabView->GetCurrentTabIndex())
	{
		CurrentAlpha += (DeltaTime / ENovaUIConstants::FadeDurationShort);
	}
	CurrentAlpha = FMath::Clamp(CurrentAlpha, 0.0f, 1.0f);

	// Hide or show the menu based on current alpha
	if (!CurrentVisible && CurrentAlpha > 0)
	{
		Show();
	}
	else if (CurrentVisible && CurrentAlpha == 0)
	{
		Hide();
	}
}

void SNovaTabPanel::OnFocusChanged(TSharedPtr<class SNovaButton> FocusButton)
{
	if (MainContainer.IsValid())
	{
		if (IsButtonInsideWidget(FocusButton, MainContainer))
		{
			MainContainer->ScrollDescendantIntoView(FocusButton, true, EDescendantScrollDestination::Center);
		}
	}
}

void SNovaTabPanel::Initialize(int32 Index, bool IsBlurred, const FSlateBrush* OptionalBackground, SNovaTabView* Parent)
{
	Blurred       = IsBlurred;
	TabIndex      = Index;
	Background    = OptionalBackground;
	ParentTabView = Parent;
}

void SNovaTabPanel::Show()
{
	CurrentVisible = true;

	NCHECK(Menu);
	Menu->SetNavigationPanel(this);
}

void SNovaTabPanel::Hide()
{
	CurrentVisible = false;

	NCHECK(Menu);
	Menu->ClearNavigationPanel();
}

bool SNovaTabPanel::IsHidden() const
{
	return (CurrentAlpha == 0);
}

TOptional<int32> SNovaTabPanel::GetBlurRadius() const
{
	const FNovaMainTheme& Theme = FNovaStyleSet::GetMainTheme();

	float CorrectedAlpha =
		FMath::Clamp((CurrentAlpha - ENovaUIConstants::BlurAlphaOffset) / (1.0f - ENovaUIConstants::BlurAlphaOffset), 0.0f, 1.0f);
	float BlurAlpha = FMath::InterpEaseInOut(0.0f, 1.0f, CorrectedAlpha, ENovaUIConstants::EaseStandard);

	return static_cast<int32>(BlurAlpha * Theme.BlurRadius);
}

float SNovaTabPanel::GetBlurStrength() const
{
	const FNovaMainTheme& Theme = FNovaStyleSet::GetMainTheme();

	float CorrectedAlpha =
		FMath::Clamp((CurrentAlpha - ENovaUIConstants::BlurAlphaOffset) / (1.0f - ENovaUIConstants::BlurAlphaOffset), 0.0f, 1.0f);
	float BlurAlpha = FMath::InterpEaseInOut(0.0f, 1.0f, CorrectedAlpha, ENovaUIConstants::EaseStandard);

	return BlurAlpha * Theme.BlurStrength;
}

/*----------------------------------------------------
    Construct
----------------------------------------------------*/

SNovaTabView::SNovaTabView() : DesiredTabIndex(0), CurrentTabIndex(0), CurrentBlurAlpha(0)
{}

void SNovaTabView::Construct(const FArguments& InArgs)
{
	// Data
	const FNovaMainTheme&   Theme       = FNovaStyleSet::GetMainTheme();
	const FNovaButtonTheme& ButtonTheme = FNovaStyleSet::GetButtonTheme();

	// clang-format off
	ChildSlot
	[
		SNew(SOverlay)

		// Fullscreen blur
		+ SOverlay::Slot()
		[
			SNew(SBox)
			.Padding(this, &SNovaTabView::GetBlurPadding)
			[
				SNew(SBackgroundBlur)
				.BlurRadius(this, &SNovaTabView::GetBlurRadius)
				.BlurStrength(this, &SNovaTabView::GetBlurStrength)
				.Padding(0)
			]
		]

		// Optional background overlay
		+ SOverlay::Slot()
		[
			SNew(SBorder)
			.BorderImage(this, &SNovaTabView::GetGlobalBackground)
			.BorderBackgroundColor(this, &SNovaTabView::GetGlobalBackgroundColor)
		]

		+ SOverlay::Slot()
		[
			SNew(SVerticalBox)

			// Header
			+ SVerticalBox::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Top)
			.AutoHeight()
			.Padding(0)
			[
				SNew(SBackgroundBlur)
				.BlurRadius(this, &SNovaTabView::GetHeaderBlurRadius)
				.BlurStrength(this, &SNovaTabView::GetHeaderBlurStrength)
				.Padding(0)
				[
					SNew(SOverlay)

					+ SOverlay::Slot()
					[
						SNew(SVerticalBox)

						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							InArgs._BackgroundWidget.Widget
						]

						+ SVerticalBox::Slot()
					]
					
					+ SOverlay::Slot()
					[
						SAssignNew(HeaderContainer, SBorder)
						.BorderImage(&Theme.MainMenuBackground)
						.Padding(0)
						.Visibility(EVisibility::SelfHitTestInvisible)
						[
							SNew(SVerticalBox)
		
							// Header buttons
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(Theme.ContentPadding)
							[
								SNew(SHorizontalBox)
				
								+ SHorizontalBox::Slot()
								.AutoWidth()
								[
									InArgs._LeftNavigation.Widget
								]

								+ SHorizontalBox::Slot()
								.AutoWidth()
								[
									SAssignNew(Header, SHorizontalBox)
								]
				
								+ SHorizontalBox::Slot()
								.AutoWidth()
								[
									InArgs._RightNavigation.Widget
								]

								+ SHorizontalBox::Slot()
				
								+ SHorizontalBox::Slot()
								.AutoWidth()
								[
									InArgs._End.Widget
								]
							]

							// User-supplied header widget
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(0)
							.HAlign(HAlign_Fill)
							[
								InArgs._Header.Widget
							]
						]
					]
				]
			]

			// Main view
			+ SVerticalBox::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			.Padding(0)
			[
				SNew(SBorder)
				.BorderImage(&Theme.MainMenuBackground)
				.ColorAndOpacity(this, &SNovaTabView::GetColor)
				.BorderBackgroundColor(this, &SNovaTabView::GetBackgroundColor)
				.Padding(0)
				[
					SAssignNew(Content, SWidgetSwitcher)
					.WidgetIndex(this, &SNovaTabView::GetCurrentTabIndex)
				]
			]
		]
	];

	// Slot contents
	int32 Index = 0;
	for (FSlot::FSlotArguments& Arg : const_cast<TArray<FSlot::FSlotArguments>&>(InArgs._Slots))
	{
		// Add header entry
		Header->AddSlot()
		.AutoWidth()
		[
			SNew(SNovaButton) // No navigation
			.Theme("TabButton")
			.Size("TabButtonSize")
			.Text(Arg._Header)
			.HelpText(Arg._HeaderHelp)
			.OnClicked(this, &SNovaTabView::SetTabIndex, Index)
			.Visibility(this, &SNovaTabView::GetTabVisibility, Index)
			.Enabled(this, &SNovaTabView::IsTabEnabled, Index)
			.Focusable(false)
		];
		

		// Add content
		SNovaTabPanel* TabPanel = static_cast<SNovaTabPanel*>(Arg.GetAttachedWidget().Get());
		TabPanel->Initialize(Index,
			Arg._Blur.IsSet() ? Arg._Blur.Get() : false,
			Arg._Background.IsSet() ? Arg._Background.Get() : nullptr, 
			this);

		Content->AddSlot()
		[
			Arg.GetAttachedWidget().ToSharedRef()
		];

		Panels.Add(TabPanel);
		PanelVisibility.Add(Arg._Visible);

		Index++;
	}

	// clang-format on
	SetVisibility(EVisibility::SelfHitTestInvisible);
}

/*----------------------------------------------------
    Interaction
----------------------------------------------------*/

void SNovaTabView::Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, CurrentTime, DeltaTime);

	// If we lost the current tab, find another one
	if (!IsTabVisible(CurrentTabIndex) && !IsTabVisible(DesiredTabIndex))
	{
		for (int32 Index = 0; Index < Panels.Num(); Index++)
		{
			int32 RelativeIndex = (Index / 2 + 1) * (Index % 2 != 0 ? 1 : -1);
			RelativeIndex       = CurrentTabIndex + (RelativeIndex % Panels.Num());

			if (RelativeIndex >= 0 && IsTabVisible(RelativeIndex))
			{
				NLOG("SNovaTabView::Tick : tab %d isn't visible, fallback to %d", CurrentTabIndex, RelativeIndex);
				SetTabIndex(RelativeIndex);
				break;
			}
		}
	}

	// Process widget change
	if (CurrentTabIndex != DesiredTabIndex)
	{
		if (GetCurrentTabContent()->IsHidden())
		{
			CurrentTabIndex = DesiredTabIndex;
		}
	}

	// Process blur
	CurrentBlurAlpha += (GetCurrentTabContent()->IsBlurred() ? 1 : -1) * (DeltaTime / ENovaUIConstants::FadeDurationMinimal);
	CurrentBlurAlpha = FMath::Clamp(CurrentBlurAlpha, 0.0f, 1.0f);
}

void SNovaTabView::ShowPreviousTab()
{
	for (int32 Index = CurrentTabIndex - 1; Index >= 0; Index--)
	{
		if (IsTabVisible(Index))
		{
			SetTabIndex(Index);
			break;
		}
	}
}

void SNovaTabView::ShowNextTab()
{
	for (int32 Index = CurrentTabIndex + 1; Index < Panels.Num(); Index++)
	{
		if (IsTabVisible(Index))
		{
			SetTabIndex(Index);
			break;
		}
	}
}

void SNovaTabView::SetTabIndex(int32 Index)
{
	NLOG("SNovaTabView::SetTabIndex : %d, was %d", Index, CurrentTabIndex);

	if (Index >= 0 && Index < Panels.Num() && Index != CurrentTabIndex)
	{
		DesiredTabIndex = Index;
	}
}

int32 SNovaTabView::GetCurrentTabIndex() const
{
	return CurrentTabIndex;
}

int32 SNovaTabView::GetDesiredTabIndex() const
{
	return DesiredTabIndex;
}

bool SNovaTabView::IsTabVisible(int32 Index) const
{
	NCHECK(Index >= 0 && Index < PanelVisibility.Num());

	if (PanelVisibility[Index].IsBound() || PanelVisibility[Index].IsSet())
	{
		return PanelVisibility[Index].Get();
	}

	return true;
}

TSharedRef<SNovaTabPanel> SNovaTabView::GetCurrentTabContent() const
{
	return SharedThis(Panels[CurrentTabIndex]);
}

/*----------------------------------------------------
    Callbacks
----------------------------------------------------*/

EVisibility SNovaTabView::GetTabVisibility(int32 Index) const
{
	return IsTabVisible(Index) ? EVisibility::Visible : EVisibility::Collapsed;
}

bool SNovaTabView::IsTabEnabled(int32 Index) const
{
	return DesiredTabIndex != Index;
}

FLinearColor SNovaTabView::GetColor() const
{
	return FLinearColor(1.0f, 1.0f, 1.0f, GetCurrentTabContent()->GetCurrentAlpha());
}

FSlateColor SNovaTabView::GetBackgroundColor() const
{
	return FLinearColor(1.0f, 1.0f, 1.0f, CurrentBlurAlpha);
}

FSlateColor SNovaTabView::GetGlobalBackgroundColor() const
{
	return FLinearColor(1.0f, 1.0f, 1.0f, GetCurrentTabContent()->GetCurrentAlpha());
}

const FSlateBrush* SNovaTabView::GetGlobalBackground() const
{
	return GetCurrentTabContent()->GetBackground();
}

bool SNovaTabView::IsBlurSplit() const
{
	return !GetCurrentTabContent()->IsBlurred() || CurrentBlurAlpha < 1.0f;
}

FMargin SNovaTabView::GetBlurPadding() const
{
	float TopPadding = IsBlurSplit() ? HeaderContainer->GetCachedGeometry().Size.Y : 0;
	return FMargin(0, TopPadding, 0, 0);
}

TOptional<int32> SNovaTabView::GetBlurRadius() const
{
	const FNovaMainTheme& Theme = FNovaStyleSet::GetMainTheme();

	float Alpha = FMath::InterpEaseInOut(0.0f, 1.0f, CurrentBlurAlpha, ENovaUIConstants::EaseStandard);
	return static_cast<int32>(Theme.BlurRadius * Alpha);
}

float SNovaTabView::GetBlurStrength() const
{
	const FNovaMainTheme& Theme = FNovaStyleSet::GetMainTheme();

	float Alpha = FMath::InterpEaseInOut(0.0f, 1.0f, CurrentBlurAlpha, ENovaUIConstants::EaseStandard);
	return Theme.BlurStrength * Alpha;
}

TOptional<int32> SNovaTabView::GetHeaderBlurRadius() const
{
	const FNovaMainTheme& Theme = FNovaStyleSet::GetMainTheme();

	return static_cast<int32>(IsBlurSplit() ? Theme.BlurRadius : 0);
}

float SNovaTabView::GetHeaderBlurStrength() const
{
	const FNovaMainTheme& Theme = FNovaStyleSet::GetMainTheme();

	return IsBlurSplit() ? Theme.BlurStrength : 0;
}
