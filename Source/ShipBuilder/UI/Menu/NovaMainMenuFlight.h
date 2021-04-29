// Spaceship Builder - Gwennaël Arbona

#pragma once

#include "UI/NovaUI.h"
#include "UI/Widget/NovaTabView.h"

#include "Online.h"

/** Flight menu */
class SNovaMainMenuFlight
	: public SNovaTabPanel
	, public INovaGameMenu
{
	/*----------------------------------------------------
	    Slate arguments
	----------------------------------------------------*/

	SLATE_BEGIN_ARGS(SNovaMainMenuFlight)
	{}

	SLATE_ARGUMENT(class SNovaMenu*, Menu)
	SLATE_ARGUMENT(TWeakObjectPtr<class UNovaMenuManager>, MenuManager)

	SLATE_END_ARGS()

public:
	SNovaMainMenuFlight();

	void Construct(const FArguments& InArgs);

	/*----------------------------------------------------
	    Interaction
	----------------------------------------------------*/

	virtual void Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime) override;

	virtual void Show() override;

	virtual void Hide() override;

	virtual void UpdateGameObjects() override;

	virtual void HorizontalAnalogInput(float Value) override;

	virtual void VerticalAnalogInput(float Value) override;

	virtual TSharedPtr<SNovaButton> GetDefaultFocusButton() const override;

	/*----------------------------------------------------
	    Content callbacks
	----------------------------------------------------*/

protected:
	bool CanFastForward() const;

	bool IsUndockEnabled() const;
	bool IsDockEnabled() const;

	bool IsManeuveringEnabled() const;

	/*----------------------------------------------------
	    Callbacks
	----------------------------------------------------*/

protected:
	void FastForward();

	void OnUndock();
	void OnDock();

	void OnAlignToManeuver();

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

protected:
	// Game objects
	TWeakObjectPtr<UNovaMenuManager>        MenuManager;
	class ANovaPlayerController*            PC;
	class ANovaSpacecraftPawn*              SpacecraftPawn;
	class UNovaSpacecraftMovementComponent* SpacecraftMovement;
	class ANovaGameState*                   GameState;
	class UNovaOrbitalSimulationComponent*  OrbitalSimulation;

	// Slate widgets
	TSharedPtr<class SNovaButton> UndockButton;
	TSharedPtr<class SNovaButton> DockButton;
	TSharedPtr<class SNovaButton> AlignManeuverButton;
	TSharedPtr<class SNovaButton> FastForwardButton;
};
