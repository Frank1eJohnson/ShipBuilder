﻿// Spaceship Builder - Gwennaël Arbona

#pragma once

#include "UI/NovaUI.h"
#include "UI/Widget/NovaTabView.h"
#include "UI/Widget/NovaModalListView.h"

class SNovaMainMenuSettings
	: public SNovaTabPanel
	, public INovaGameMenu
{
	/*----------------------------------------------------
	    Slate arguments
	----------------------------------------------------*/

	SLATE_BEGIN_ARGS(SNovaMainMenuSettings)
	{}

	SLATE_ARGUMENT(class SNovaMenu*, Menu)
	SLATE_ARGUMENT(TWeakObjectPtr<class UNovaMenuManager>, MenuManager)

	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs);

	/*----------------------------------------------------
	    Interaction
	----------------------------------------------------*/

	virtual void Show() override;

	TSharedPtr<class SNovaButton> GetDefaultFocusButton() const override;

	/*----------------------------------------------------
	    Input management
	----------------------------------------------------*/

	void OnKeyBindingChanged(FKey PreviousKey, FKey NewKey, TSharedPtr<struct FNovaKeyBinding> Binding);
	void OnKeyBindingReset(TSharedPtr<struct FNovaKeyBinding> Binding);

	void ApplyNewBinding(TSharedPtr<struct FNovaKeyBinding> BindingThatChanged, bool Replace);
	void CancelNewBinding(TSharedPtr<struct FNovaKeyBinding> BindingThatChanged, FKey PreviousKey);
	bool IsAlreadyUsed(TArray<TSharedPtr<struct FNovaKeyBinding>>& BindConflicts, FKey Key, TSharedPtr<FNovaKeyBinding> ExcludeBinding);

	/*----------------------------------------------------
	    Video management
	----------------------------------------------------*/

	TSharedPtr<class SNovaSlider> AddSettingSlider(TSharedPtr<class SVerticalBox> Container, FText Text, FText HelpText,
		FOnFloatValueChanged Callback, float MinValue = 0, float MaxValue = 4, float ValueStep = 1, TAttribute<bool> Enabled = true);

	void UpdateResolutionList();

	void UpdateResolution(FIntPoint Resolution, bool Fullscreen);

	void ApplyVideoSettings();
	void RevertVideoSettings();

	/*----------------------------------------------------
	    Callbacks
	----------------------------------------------------*/

protected:
	// Are we on the PC platform
	EVisibility GetPCVisibility() const;
	bool        IsScreenPercentageSupported() const;
	bool        IsDLSSSupported() const;
	bool        IsHDRSupported() const;
	bool        IsRaytracingSupported() const;

	// Gameplay settings callbacks
	void OnFOVChanged(float Value);
	void OnCrashReportToggled();

	// Graphics settings callbacks
	void OnViewDistanceChanged(float Value);
	void OnShadowChanged(float Value);
	void OnEffectsChanged(float Value);
	void OnPostProcessChanged(float Value);
	void OnAntiAliasingChanged(float Value);
	void OnScreenPercentageChanged(float Value);
	void OnDLSSToggled();
	void OnNaniteToggled();
	void OnLumenToggled();
	void OnLumenHWRTToggled();
	void OnVirtualShadowsToggled();
	void OnCinematicBloomToggled();

	// Culture list
	TSharedRef<SWidget> GenerateCultureItem(TSharedPtr<FString> Culture);
	FText               GetCultureName(TSharedPtr<FString> Culture) const;
	FText               GenerateCultureTooltip(TSharedPtr<FString> Culture);
	void                OnSelectedCultureChanged(TSharedPtr<FString> Culture, int32 Index);

	// Culture settings
	void OnApplyCultureSettings();

	// Resolution list
	TSharedRef<SWidget> GenerateResolutionItem(TSharedPtr<struct FScreenResolutionRHI> Resolution);
	FText               GetResolutionName(TSharedPtr<struct FScreenResolutionRHI> Resolution) const;
	FText               GenerateResolutionTooltip(TSharedPtr<struct FScreenResolutionRHI> Resolution);
	void                OnSelectedResolutionChanged(TSharedPtr<struct FScreenResolutionRHI> Resolution, int32 Index);

	// Video settings
	bool IsApplyVideoSettingsEnabled() const;
	void OnApplyVideoSettings();

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

protected:
	// Menu widgets
	TWeakObjectPtr<UNovaMenuManager>  MenuManager;
	TSharedPtr<class SVerticalBox>    BindingsContainer;
	TSharedPtr<class SNovaModalPanel> ModalPanel;

	// Data
	class UNovaGameUserSettings*                    GameUserSettings;
	TArray<TSharedPtr<FString>>                     CultureList;
	TSharedPtr<FString>                             SelectedCulture;
	TArray<TSharedPtr<struct FScreenResolutionRHI>> ResolutionList;
	TSharedPtr<struct FScreenResolutionRHI>         SelectedResolution;
	TArray<TSharedPtr<struct FNovaKeyBinding>>      Bindings;
	FIntPoint                                       LastConfirmedVideoResolution;
	EWindowMode::Type                               LastConfirmedFullscreenMode;

	// Game settings widgets
	TSharedPtr<SVerticalBox>                            GameplayContainer;
	TSharedPtr<SNovaModalListView<TSharedPtr<FString>>> CultureListView;
	TSharedPtr<class SNovaSlider>                       FOVSlider;
	TSharedPtr<class SNovaButton>                       CrashReportButton;

	// Display settings widgets
	TSharedPtr<SNovaModalListView<TSharedPtr<struct FScreenResolutionRHI>>> ResolutionListView;
	TSharedPtr<class SNovaButton>                                           FullscreenButton;
	TSharedPtr<class SNovaButton>                                           VSyncButton;
	TSharedPtr<class SNovaButton>                                           HDRButton;

	// Graphics settings widgets
	TSharedPtr<SVerticalBox>      GraphicsContainer;
	TSharedPtr<class SNovaSlider> ViewDistanceSlider;
	TSharedPtr<class SNovaSlider> ShadowSlider;
	TSharedPtr<class SNovaSlider> EffectsSlider;
	TSharedPtr<class SNovaSlider> PostProcessSlider;
	TSharedPtr<class SNovaSlider> AntiAliasingSlider;
	TSharedPtr<class SNovaSlider> ScreenPercentageSlider;
	TSharedPtr<class SNovaButton> DLSSButton;
	TSharedPtr<class SNovaButton> NaniteButton;
	TSharedPtr<class SNovaButton> LumenButton;
	TSharedPtr<class SNovaButton> LumenHWRTButton;
	TSharedPtr<class SNovaButton> VirtualShadowsButton;
	TSharedPtr<class SNovaButton> CinematicBloomButton;
};
