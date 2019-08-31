// Nova project - Gwennaël Arbona

#pragma once

#include "NovaMenu.h"
#include "NovaButton.h"
#include "NovaNavigationPanel.h"
#include "Nova/UI/NovaUI.h"
#include "Nova/Nova.h"

#include "Widgets/SCompoundWidget.h"

/** Templatized list class to display elements from a TArray */
template <typename ItemType>
class SNovaListView : public SCompoundWidget
{
	DECLARE_DELEGATE_RetVal_OneParam(TSharedRef<SWidget>, FNovaOnGenerateItem, ItemType);
	DECLARE_DELEGATE_RetVal_OneParam(FText, FNovaOnGenerateTooltip, ItemType);
	DECLARE_DELEGATE_TwoParams(FNovaListSelectionChanged, ItemType, int32);

	/*----------------------------------------------------
	    Slate arguments
	----------------------------------------------------*/

	SLATE_BEGIN_ARGS(SNovaListView<ItemType>) : _Horizontal(false), _ButtonTheme("DefaultButton"), _ButtonSize("DoubleButtonSize")
	{}

	SLATE_ARGUMENT(SNovaNavigationPanel*, Panel)
	SLATE_ARGUMENT(const TArray<ItemType>*, ItemsSource)

	SLATE_EVENT(FNovaOnGenerateItem, OnGenerateItem)
	SLATE_EVENT(FNovaOnGenerateTooltip, OnGenerateTooltip)
	SLATE_EVENT(FNovaListSelectionChanged, OnSelectionChanged)
	SLATE_EVENT(FSimpleDelegate, OnSelectionDoubleClicked)

	SLATE_ARGUMENT(bool, Horizontal)
	SLATE_ARGUMENT(FName, ButtonTheme)
	SLATE_ARGUMENT(FName, ButtonSize)

	SLATE_END_ARGS()

	/*----------------------------------------------------
	    Constructor
	----------------------------------------------------*/

public:
	void Construct(const FArguments& InArgs)
	{
		const FNovaMainTheme& Theme = FNovaStyleSet::GetMainTheme();

		// Setup
		Panel                    = InArgs._Panel;
		ItemsSource              = InArgs._ItemsSource;
		OnGenerateItem           = InArgs._OnGenerateItem;
		OnGenerateTooltip        = InArgs._OnGenerateTooltip;
		OnSelectionChanged       = InArgs._OnSelectionChanged;
		OnSelectionDoubleClicked = InArgs._OnSelectionDoubleClicked;
		ButtonTheme              = InArgs._ButtonTheme;
		ButtonSize               = InArgs._ButtonSize;

		// Sanity checks
		NCHECK(Panel);
		NCHECK(ItemsSource);

		// clang-format off
		ChildSlot
		.VAlign(VAlign_Fill)
		.HAlign(HAlign_Center)
		[
			SAssignNew(Container, SScrollBox)
			.AnimateWheelScrolling(true)
			.Style(&Theme.ScrollBoxStyle)
			.ScrollBarVisibility(EVisibility::Collapsed)
			.Orientation(InArgs._Horizontal ? Orient_Horizontal : Orient_Vertical)
		];
		// clang-format on

		// Initialize
		CurrentSelectedIndex   = 0;
		InitiallySelectedIndex = 0;
	}

	/*----------------------------------------------------
	    Public methods
	----------------------------------------------------*/

public:
	/** Refresh the list based on the items source */
	void Refresh(int32 SelectedIndex = INDEX_NONE)
	{
		SelectedIndex          = FMath::Min(SelectedIndex, ItemsSource->Num() - 1);
		InitiallySelectedIndex = SelectedIndex;

		// Get the state of the focused button
		int32            PreviousSelectedIndex       = INDEX_NONE;
		int32            FocusButtonIndex            = 0;
		FNovaButtonState PreviousSelectedButtonState = FNovaButtonState();
		for (TSharedPtr<SNovaButton> Button : ListButtons)
		{
			if (Button->IsFocused())
			{
				PreviousSelectedButtonState = Button->GetState();
				PreviousSelectedIndex       = FocusButtonIndex;
				break;
			}

			FocusButtonIndex++;
		}

		// Clear UI
		for (TSharedPtr<SNovaButton> Button : ListButtons)
		{
			Panel->DestroyFocusableButton(Button);
		}
		Container->ClearChildren();
		ListButtons.Empty();

		// Build new UI
		int32 BuildIndex = 0;
		for (ItemType Item : *ItemsSource)
		{
			// Add buttons
			TSharedPtr<SNovaButton> Button;
			// clang-format off
			Container->AddSlot()
			[
				Panel->SNovaAssignNew(Button, SNovaButton)
				.Theme(ButtonTheme)
				.Size(ButtonSize)
				.OnFocused(this, &SNovaListView<ItemType>::OnElementSelected, Item, BuildIndex)
				.OnClicked(this, &SNovaListView<ItemType>::OnElementSelected, Item, BuildIndex)
				.OnDoubleClicked(FSimpleDelegate::CreateLambda([=]()
				{
					OnSelectionDoubleClicked.ExecuteIfBound();
				}))
			];
			// clang-format on

			// Fill tooltip
			if (OnGenerateTooltip.IsBound())
			{
				Button->SetHelpText(OnGenerateTooltip.Execute(Item));
			}

			// Fill contents
			BuildIndex++;
			if (OnGenerateItem.IsBound())
			{
				Button->GetContainer()->SetContent(OnGenerateItem.Execute(Item));
			}

			ListButtons.Add(Button);
		}

		// Refresh navigation
		Panel->GetMenu()->RefreshNavigationPanel();

		if (SelectedIndex != INDEX_NONE)
		{
			CurrentSelectedIndex = SelectedIndex;
			Panel->GetMenu()->SetFocusedButton(ListButtons[CurrentSelectedIndex], true);
		}
		else if (PreviousSelectedIndex != INDEX_NONE)
		{
			CurrentSelectedIndex = FMath::Min(CurrentSelectedIndex, ItemsSource->Num() - 1);

			PreviousSelectedIndex = FMath::Clamp(PreviousSelectedIndex, 0, BuildIndex - 1);
			Panel->GetMenu()->SetFocusedButton(ListButtons[PreviousSelectedIndex], true);
			ListButtons[PreviousSelectedIndex]->GetState() = PreviousSelectedButtonState;
		}
	}

	/** Force the initially selected index */
	void SetInitiallySelectedIndex(int32 Index)
	{
		InitiallySelectedIndex = Index;
	}

	/** Check if an item is currently selected */
	bool IsCurrentlySelected(const ItemType& Item) const
	{
		return Item == GetSelectedItem();
	}

	/** Get the selection icon */
	const FSlateBrush* GetSelectionIcon(const ItemType& Item) const
	{
		bool IsInitialItem = InitiallySelectedIndex >= 0 && ItemsSource && InitiallySelectedIndex < ItemsSource->Num() &&
							 Item == (*ItemsSource)[InitiallySelectedIndex];

		return FNovaStyleSet::GetBrush(IsInitialItem ? "Icon/SB_ListOn" : "Icon/SB_ListOff");
	}

	/** Get the currently selected index */
	int32 GetSelectedIndex() const
	{
		return CurrentSelectedIndex;
	}

	/** Get the currently selected item */
	const ItemType& GetSelectedItem() const
	{
		NCHECK(CurrentSelectedIndex >= 0 && CurrentSelectedIndex < (*ItemsSource).Num());
		return (*ItemsSource)[CurrentSelectedIndex];
	}

	/*----------------------------------------------------
	    Callbacks
	----------------------------------------------------*/

protected:
	/** New list item was selected */
	void OnElementSelected(ItemType Selected, int32 Index)
	{
		CurrentSelectedIndex = Index;

		OnSelectionChanged.ExecuteIfBound(Selected, Index);
		Container->ScrollDescendantIntoView(ListButtons[Index], true, EDescendantScrollDestination::IntoView);
	}

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

protected:
	// Settings
	SNovaNavigationPanel*     Panel;
	const TArray<ItemType>*   ItemsSource;
	FNovaOnGenerateItem       OnGenerateItem;
	FNovaOnGenerateTooltip    OnGenerateTooltip;
	FNovaListSelectionChanged OnSelectionChanged;
	FSimpleDelegate           OnSelectionDoubleClicked;
	FName                     ButtonTheme;
	FName                     ButtonSize;

	// State
	int32 CurrentSelectedIndex;
	int32 InitiallySelectedIndex;

	// Widgets
	TSharedPtr<SScrollBox>          Container;
	TArray<TSharedPtr<SNovaButton>> ListButtons;
};
