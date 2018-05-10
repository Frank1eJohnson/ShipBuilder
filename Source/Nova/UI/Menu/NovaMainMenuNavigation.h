// Nova project - Gwennaël Arbona

#pragma once

#include "Nova/UI/NovaUI.h"
#include "Nova/UI/Widget/NovaTabView.h"
#include "Nova/UI/Widget/NovaModalListView.h"

#include "Online.h"

/** Navigation menu */
class SNovaMainMenuNavigation : public SNovaTabPanel
{
	/*----------------------------------------------------
	    Slate arguments
	----------------------------------------------------*/

	SLATE_BEGIN_ARGS(SNovaMainMenuNavigation)
	{}

	SLATE_ARGUMENT(class SNovaMenu*, Menu)
	SLATE_ARGUMENT(TWeakObjectPtr<class UNovaMenuManager>, MenuManager)

	SLATE_END_ARGS()

public:
	SNovaMainMenuNavigation() : SelectedDestination(nullptr)
	{}

	void Construct(const FArguments& InArgs);

	/*----------------------------------------------------
	    Interaction
	----------------------------------------------------*/

	virtual void Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime) override;

	virtual void Show() override;

	virtual void Hide() override;

	/*----------------------------------------------------
	    Internals
	----------------------------------------------------*/

protected:
	/** Get the spacecraft pawn */
	class ANovaSpacecraftPawn* GetSpacecraftPawn() const;

	/** Get the spacecraft movement component */
	class UNovaSpacecraftMovementComponent* GetSpacecraftMovement() const;

	/** Check for trajectory validity */
	bool CanCommitTrajectoryInternal(FText* Help = nullptr) const;

	/*----------------------------------------------------
	    Callbacks
	----------------------------------------------------*/

protected:
	// Destinations
	TSharedRef<SWidget> GenerateDestinationItem(const class UNovaArea* Destination);
	FText               GetDestinationName(const class UNovaArea* Destination) const;
	const FSlateBrush*  GetDestinationIcon(const class UNovaArea* Destination) const;
	FText               GenerateDestinationTooltip(const class UNovaArea* Destination);
	void                OnSelectedDestinationChanged(const class UNovaArea* Destination, int32 Index);

	// General
	FText GetLocationText() const;
	bool  CanFastForward() const;
	void  FastForward();

	// Trajectory
	bool  CanCommitTrajectory() const;
	FText GetCommitTrajectoryHelpText() const;
	void  OnTrajectoryChanged(TSharedPtr<struct FNovaTrajectory> Trajectory);
	void  OnCommitTrajectory();

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

protected:
	// Settings
	TWeakObjectPtr<UNovaMenuManager> MenuManager;

	// Destination list
	const class UNovaArea*                           SelectedDestination;
	TArray<const class UNovaArea*>                   DestinationList;
	TSharedPtr<SNovaModalListView<const UNovaArea*>> DestinationListView;

	// Slate widgets
	TSharedPtr<class SNovaOrbitalMap>           OrbitalMap;
	TSharedPtr<class SNovaTrajectoryCalculator> TrajectoryCalculator;
};