// Spaceship Builder - Gwennaël Arbona

#include "NovaPostProcessComponent.h"
#include "Game/NovaGameUserSettings.h"
#include "UI/NovaUITypes.h"
#include "Nova.h"

#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/Material.h"
#include "Components/PostProcessComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "Engine.h"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

UNovaPostProcessComponent::UNovaPostProcessComponent() : Super(), CurrentPreset(0), TargetPreset(0), CurrentPresetAlpha(0.0f)
{
	// Resources
	static ConstructorHelpers::FClassFinder<AActor> PostProcessActorClassRef(TEXT("/Game/Environment/BP_PostProcess"));
	PostProcessActorClass = PostProcessActorClassRef.Class;

	// Settings
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(false);
}

/*----------------------------------------------------
    Gameplay
----------------------------------------------------*/

void UNovaPostProcessComponent::Initialize(FNovaPostProcessControl Control, FNovaPostProcessUpdate Update)
{
	ControlFunction = Control;
	UpdateFunction  = Update;
}

void UNovaPostProcessComponent::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PC = GetOwner<APlayerController>();
	NCHECK(PC);

	if (PC && PC->IsLocalController())
	{
		TArray<AActor*> PostProcessActors;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), PostProcessActorClass, PostProcessActors);

		// Set the post process
		if (PostProcessActors.Num())
		{
			PostProcessVolume =
				Cast<UPostProcessComponent>(PostProcessActors[0]->GetComponentByClass(UPostProcessComponent::StaticClass()));

			// Replace the material by a dynamic variant
			if (PostProcessVolume)
			{
				TArray<FWeightedBlendable> Blendables = PostProcessVolume->Settings.WeightedBlendables.Array;
				PostProcessVolume->Settings.WeightedBlendables.Array.Empty();

				for (FWeightedBlendable Blendable : Blendables)
				{
					UMaterialInterface* BaseMaterial = Cast<UMaterialInterface>(Blendable.Object);
					NCHECK(BaseMaterial);

					UMaterialInstanceDynamic* MaterialInstance = UMaterialInstanceDynamic::Create(BaseMaterial, GetWorld());
					if (!IsValid(PostProcessMaterial))
					{
						PostProcessMaterial = MaterialInstance;
					}

					PostProcessVolume->Settings.AddBlendable(MaterialInstance, 1.0f);
				}

				NLOG("UNovaPostProcessComponent::BeginPlay : post-process setup complete");
			}
		}
	}
}

void UNovaPostProcessComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	APlayerController* PC = GetOwner<APlayerController>();
	NCHECK(PC);

	if (PC->IsLocalController() && PostProcessVolume)
	{
		// Update desired settings
		if (ControlFunction.IsBound() && CurrentPreset == TargetPreset)
		{
			TargetPreset = ControlFunction.Execute();
		}

		// Update transition time
		float CurrentTransitionDuration = PostProcessSettings[TargetPreset]->TransitionDuration;
		if (CurrentPreset != TargetPreset)
		{
			CurrentPresetAlpha -= DeltaTime / CurrentTransitionDuration;
		}
		else if (CurrentPreset != 0)
		{
			CurrentPresetAlpha += DeltaTime / CurrentTransitionDuration;
		}
		CurrentPresetAlpha = FMath::Clamp(CurrentPresetAlpha, 0.0f, 1.0f);

		// Manage state transitions
		if (CurrentPresetAlpha <= 0)
		{
			CurrentPreset = TargetPreset;
		}

		// Get post process targets and apply the new settings
		float InterpolatedAlpha = FMath::InterpEaseInOut(0.0f, 1.0f, CurrentPresetAlpha, ENovaUIConstants::EaseStandard);
		TSharedPtr<FNovaPostProcessSettingBase>& CurrentPostProcess = PostProcessSettings[0];
		TSharedPtr<FNovaPostProcessSettingBase>& TargetPostProcess  = PostProcessSettings[CurrentPreset];
		UpdateFunction.ExecuteIfBound(PostProcessVolume, PostProcessMaterial, CurrentPostProcess, TargetPostProcess, CurrentPresetAlpha);
	}
}
