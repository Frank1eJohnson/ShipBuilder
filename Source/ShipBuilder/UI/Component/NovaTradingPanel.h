// Spaceship Builder - Gwennaël Arbona

#pragma once

#include "UI/Widget/NovaModalPanel.h"

/** Trading panel */
class SNovaTradingPanel : public SNovaModalPanel
{
	/*----------------------------------------------------
	    Slate args
	----------------------------------------------------*/

	SLATE_BEGIN_ARGS(SNovaTradingPanel)
	{}

	SLATE_ARGUMENT(class SNovaMenu*, Menu)

	SLATE_END_ARGS()

public:
	SNovaTradingPanel() : Spacecraft(nullptr), Resource(nullptr), CompartmentIndex(INDEX_NONE)
	{}

	void Construct(const FArguments& InArgs);

	/*----------------------------------------------------
	    Interface
	----------------------------------------------------*/

public:
	/** Show contents without trading */
	void Inspect(ANovaPlayerController* TargetPC, const UNovaResource* TargetResource, int32 TargetCompartmentIndex)
	{
		ShowPanelInternal(TargetPC, TargetResource, TargetCompartmentIndex, false);
	}

	/** Start trading */
	void Trade(class ANovaPlayerController* TargetPC, const class UNovaResource* TargetResource, int32 TargetCompartmentIndex)
	{
		ShowPanelInternal(TargetPC, TargetResource, TargetCompartmentIndex, true);
	}

protected:
	/** Implementation of the modal panel */
	void ShowPanelInternal(
		class ANovaPlayerController* TargetPC, const class UNovaResource* TargetResource, int32 TargetCompartmentIndex, bool AllowTrade);

	/*----------------------------------------------------
	    Content callbacks
	----------------------------------------------------*/

protected:
	bool IsConfirmEnabled() const override;

	FText GetResourceDetails() const;

	TOptional<float> GetCargoProgress() const;

	FText GetCargoAmount() const;
	FText GetCargoMinValue() const;
	FText GetCargoMaxValue() const;
	FText GetCargoDetails() const;

	FText            GetTransactionDetails() const;
	ENovaInfoBoxType GetTransactionType() const;
	FNovaCredits     GetTransactionValue() const;

	/*----------------------------------------------------
	    Callbacks
	----------------------------------------------------*/

protected:
	/** Confirm the trade and proceed */
	void OnConfirmTrade();

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

protected:
	// Slate widgets
	TSharedPtr<class SNovaSlider>            AmountSlider;
	TSharedPtr<class SNovaTradableAssetItem> ResourceItem;
	TSharedPtr<class SHorizontalBox>         CaptionBox;
	TSharedPtr<class SNovaInfoText>          InfoText;

	// Data
	TWeakObjectPtr<ANovaPlayerController> PC;
	const struct FNovaSpacecraft*         Spacecraft;
	const UNovaResource*                  Resource;
	float                                 InitialAmount;
	float                                 MinAmount;
	float                                 MaxAmount;
	float                                 Capacity;
	int32                                 CompartmentIndex;
	bool                                  IsTradeAllowed;
};
