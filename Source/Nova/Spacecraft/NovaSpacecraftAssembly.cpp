// Nova project - Gwennaël Arbona

#include "NovaSpacecraftAssembly.h"
#include "NovaSpacecraftMovementComponent.h"
#include "NovaCompartmentAssembly.h"

#include "Nova/Actor/NovaMeshInterface.h"

#include "Nova/Game/NovaGameInstance.h"
#include "Nova/Game/NovaAssetCatalog.h"
#include "Nova/Player/NovaPlayerController.h"

#include "Nova/Nova.h"


#define LOCTEXT_NAMESPACE "ANovaAssembly"


/*----------------------------------------------------
	Constructor
----------------------------------------------------*/

ANovaSpacecraftAssembly::ANovaSpacecraftAssembly()
	: Super()
	, AssemblyState(ENovaAssemblyState::Idle)
	, WaitingAssetLoading(false)
	, CurrentHighlightCompartment(INDEX_NONE)
	, DisplayFilterType(ENovaAssemblyDisplayFilter::All)
	, DisplayFilterIndex(INDEX_NONE)
	, ImmediateMode(false)
{
	// Setup movement component
	MovementComponent = CreateDefaultSubobject<UNovaSpacecraftMovementComponent>(TEXT("MovementComponent"));
	MovementComponent->SetUpdatedComponent(RootComponent);

	// Settings
	bAlwaysRelevant = true;
	PrimaryActorTick.bCanEverTick = true;
}


/*----------------------------------------------------
	Loading & saving
----------------------------------------------------*/

void ANovaSpacecraftAssembly::SerializeJson(TSharedPtr<FNovaSpacecraft>& SaveData, TSharedPtr<FJsonObject>& JsonData, ENovaSerialize Direction)
{
	FNovaSpacecraft::SerializeJson(SaveData, JsonData, Direction);
}


/*----------------------------------------------------
	Multiplayer
----------------------------------------------------*/

bool ANovaSpacecraftAssembly::ServerSetSpacecraft_Validate(const FNovaSpacecraft& NewSpacecraft)
{
	return true;
}

void ANovaSpacecraftAssembly::ServerSetSpacecraft_Implementation(const FNovaSpacecraft& NewSpacecraft)
{
	NLOG("ANovaSpacecraftAssembly::ServerSetSpacecraft");

	ServerSpacecraft = NewSpacecraft;

	SetSpacecraft(NewSpacecraft.GetSharedCopy());
}

void ANovaSpacecraftAssembly::OnServerSpacecraftReplicated()
{
	NLOG("ANovaSpacecraftAssembly::OnServerSpacecraftReplicated");

	SetSpacecraft(ServerSpacecraft.GetSharedCopy());
}

void ANovaSpacecraftAssembly::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANovaSpacecraftAssembly, ServerSpacecraft);
}


/*----------------------------------------------------
	Gameplay
----------------------------------------------------*/

void ANovaSpacecraftAssembly::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Fetch the spacecraft from the player save if we don't have one
	if (!Spacecraft.IsValid())
	{
		ANovaPlayerController* PC = GetController<ANovaPlayerController>();
		if (PC)
		{
			const TSharedPtr<FNovaSpacecraft>& PlayerSpacecraft = GetController<ANovaPlayerController>()->GetSpacecraft();
			if (PlayerSpacecraft.IsValid())
			{
				SetSpacecraft(PlayerSpacecraft);
				ServerSpacecraft = *PlayerSpacecraft;
			}
		}
	}

	// Assembly sequence
	else if (AssemblyState != ENovaAssemblyState::Idle)
	{
		UpdateAssembly();
	}

	// Idle processing
	else
	{
		// Update selection
		for (int32 CompartmentIndex = 0; CompartmentIndex < CompartmentAssemblies.Num(); CompartmentIndex++)
		{
			ProcessCompartment(
				CompartmentAssemblies[CompartmentIndex],
				FNovaCompartment(),
				FNovaAssemblyCallback::CreateLambda([=](FNovaAssemblyElement& Element, TSoftObjectPtr<UObject> Asset, FNovaAdditionalComponent AdditionalComponent)
					{
						UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(Element.Mesh);
						if (PrimitiveComponent)
						{
							int32 Value = CompartmentIndex == CurrentHighlightCompartment ? 1 : 0;
							PrimitiveComponent->SetCustomDepthStencilValue(Value);
						}
					}
			));
		}
	}
}

TArray<const UNovaCompartmentDescription*> ANovaSpacecraftAssembly::GetCompatibleCompartments(int32 Index) const
{
	TArray<const UNovaCompartmentDescription*> CompartmentDescriptions;

	for (const UNovaCompartmentDescription* Description : UNovaAssetCatalog::Get()->GetAssets<UNovaCompartmentDescription>())
	{
		CompartmentDescriptions.Add(Description);
	}

	return CompartmentDescriptions;
}

TArray<const class UNovaModuleDescription*> ANovaSpacecraftAssembly::GetCompatibleModules(int32 Index, int32 SlotIndex) const
{
	TArray<const UNovaModuleDescription*> ModuleDescriptions;
	TArray<const UNovaModuleDescription*> AllModuleDescriptions = UNovaAssetCatalog::Get()->GetAssets<UNovaModuleDescription>();
	const FNovaCompartment& CompartmentAssembly = Spacecraft->Compartments[Index];

	ModuleDescriptions.Add(nullptr);
	if (CompartmentAssembly.IsValid() && SlotIndex < CompartmentAssembly.Description->ModuleSlots.Num())
	{
		for (const UNovaModuleDescription* ModuleDescription : AllModuleDescriptions)
		{
			ModuleDescriptions.AddUnique(ModuleDescription);
		}
	}

	return ModuleDescriptions;
}

TArray<const UNovaEquipmentDescription*> ANovaSpacecraftAssembly::GetCompatibleEquipments(int32 Index, int32 SlotIndex) const
{
	TArray<const UNovaEquipmentDescription*> EquipmentDescriptions;
	TArray<const UNovaEquipmentDescription*> AllEquipmentDescriptions = UNovaAssetCatalog::Get()->GetAssets<UNovaEquipmentDescription>();
	const FNovaCompartment& CompartmentAssembly = Spacecraft->Compartments[Index];

	EquipmentDescriptions.Add(nullptr);
	if (CompartmentAssembly.IsValid() && SlotIndex < CompartmentAssembly.Description->EquipmentSlots.Num())
	{
		for (const UNovaEquipmentDescription* EquipmentDescription : AllEquipmentDescriptions)
		{
			const TArray<ENovaEquipmentType>& SupportedTypes = CompartmentAssembly.Description->EquipmentSlots[SlotIndex].SupportedTypes;
			if (SupportedTypes.Num() == 0 || SupportedTypes.Contains(EquipmentDescription->EquipmentType))
			{
				EquipmentDescriptions.AddUnique(EquipmentDescription);
			}
		}
	}

	return EquipmentDescriptions;
}

void ANovaSpacecraftAssembly::SaveAssembly()
{
	NCHECK(Spacecraft.IsValid());

	ServerSetSpacecraft(Spacecraft->GetSafeCopy());

	GetController<ANovaPlayerController>()->GetGameInstance<UNovaGameInstance>()->SaveGame();
}

void ANovaSpacecraftAssembly::SetSpacecraft(const TSharedPtr<FNovaSpacecraft> NewSpacecraft)
{
	if (AssemblyState == ENovaAssemblyState::Idle)
	{
		NCHECK(NewSpacecraft.IsValid());
		NLOG("ANovaAssembly::SetSpacecraft (%d compartments)", NewSpacecraft->Compartments.Num());

		// Clean up first
		if (Spacecraft)
		{
			for (int32 Index = 0; Index < Spacecraft->Compartments.Num(); Index++)
			{
				RemoveCompartment(Index);
			}
		}

		// Initialize physical compartments
		for (const FNovaCompartment& Assembly : NewSpacecraft->Compartments)
		{
			NCHECK(Assembly.Description);
			CompartmentAssemblies.Add(CreateCompartment(Assembly));
		}

		// Start assembling, using a copy of the target assembly data
		Spacecraft = NewSpacecraft;
		StartAssemblyUpdate();
	}
}

bool ANovaSpacecraftAssembly::InsertCompartment(FNovaCompartment Compartment, int32 Index)
{
	NLOG("ANovaAssembly::InsertCompartment %d", Index);

	if (AssemblyState == ENovaAssemblyState::Idle)
	{
		NCHECK(Spacecraft.IsValid());
		NCHECK(Compartment.Description);

		Spacecraft->Compartments.Insert(Compartment, Index);
		CompartmentAssemblies.Insert(CreateCompartment(Compartment), Index);

		return true;
	}

	return false;
}

bool ANovaSpacecraftAssembly::RemoveCompartment(int32 Index)
{
	NLOG("ANovaAssembly::RemoveCompartment %d", Index);

	if (AssemblyState == ENovaAssemblyState::Idle)
	{
		NCHECK(Spacecraft.IsValid());
		NCHECK(Index >= 0 && Index < Spacecraft->Compartments.Num());
		Spacecraft->Compartments[Index] = FNovaCompartment();

		return true;
	}

	return false;
}

FNovaCompartment& ANovaSpacecraftAssembly::GetCompartment(int32 Index)
{
	NCHECK(Spacecraft.IsValid());
	NCHECK(Index >= 0 && Index < Spacecraft->Compartments.Num());
	return Spacecraft->Compartments[Index];
}

int32 ANovaSpacecraftAssembly::GetCompartmentCount() const
{
	int32 Count = 0;

	if (Spacecraft)
	{
		for (const FNovaCompartment& Assembly : Spacecraft->Compartments)
		{
			if (Assembly.IsValid())
			{
				Count++;
			}
		}
	}

	return Count;
}

int32 ANovaSpacecraftAssembly::GetCompartmentIndexByPrimitive(const class UPrimitiveComponent* Component)
{
	int32 Result = 0;
	for (UNovaCompartmentAssembly* Compartment : CompartmentAssemblies)
	{
		if (Component->IsAttachedTo(Compartment))
		{
			return Result;
		}
		Result++;
	}

	return INDEX_NONE;
}

void ANovaSpacecraftAssembly::SetDisplayFilter(ENovaAssemblyDisplayFilter Filter, int32 CompartmentIndex)
{
	DisplayFilterType = Filter;
	DisplayFilterIndex = CompartmentIndex;

	if (AssemblyState == ENovaAssemblyState::Idle)
	{
		UpdateDisplayFilter();
		UpdateBounds();
	}
}


/*----------------------------------------------------
	Compartment assembly internals
----------------------------------------------------*/

void ANovaSpacecraftAssembly::StartAssemblyUpdate()
{
	NCHECK(AssemblyState == ENovaAssemblyState::Idle);

	// In case we have nothing to de-materialize or load, we will move there
	AssemblyState = ENovaAssemblyState::Moving;

	Spacecraft->UpdateProceduralElements();

	// De-materialize unwanted compartments
	for (int32 CompartmentIndex = 0; CompartmentIndex < CompartmentAssemblies.Num(); CompartmentIndex++)
	{
		ProcessCompartmentIfDifferent(
		CompartmentAssemblies[CompartmentIndex],
		CompartmentIndex < Spacecraft->Compartments.Num() ? Spacecraft->Compartments[CompartmentIndex] : FNovaCompartment(),
		FNovaAssemblyCallback::CreateLambda([=](FNovaAssemblyElement& Element, TSoftObjectPtr<UObject> Asset, FNovaAdditionalComponent AdditionalComponent)
			{
				if (Element.Mesh)
				{
					Element.Mesh->Dematerialize(ImmediateMode);
					AssemblyState = ENovaAssemblyState::LoadingDematerializing;
				}
			}
		));
	}

	// Find new assets to load
	NCHECK(RequestedAssets.Num() == 0);
	for (const FNovaCompartment& Compartment : Spacecraft->Compartments)
	{
		if (Compartment.Description)
		{
			for (FSoftObjectPath Asset : Compartment.Description->GetAsyncAssets())
			{
				RequestedAssets.AddUnique(Asset);
			}
			for (const FNovaCompartmentModule& Module : Compartment.Modules)
			{
				if (Module.Description)
				{
					for (FSoftObjectPath Asset : Module.Description->GetAsyncAssets())
					{
						RequestedAssets.AddUnique(Asset);
					}
				}
			}
			for (const UNovaEquipmentDescription* Equipment : Compartment.Equipments)
			{
				if (Equipment)
				{
					for (FSoftObjectPath Asset : Equipment->GetAsyncAssets())
					{
						RequestedAssets.AddUnique(Asset);
					}
				}
			}
		}
	}

	// Request loading of new assets
	if (RequestedAssets.Num())
	{
		if (ImmediateMode)
		{
			UNovaAssetCatalog::Get()->LoadAssets(RequestedAssets);
		}
		else
		{
			AssemblyState = ENovaAssemblyState::LoadingDematerializing;
			WaitingAssetLoading = true;

			UNovaAssetCatalog::Get()->LoadAssets(RequestedAssets,
				FStreamableDelegate::CreateLambda([=]()
					{
						WaitingAssetLoading = false;
					}
			));
		}
	}
}

void ANovaSpacecraftAssembly::UpdateAssembly()
{
	// Process de-materialization and destruction of previous components, wait asset loading
	if (AssemblyState == ENovaAssemblyState::LoadingDematerializing)
	{
		bool StillWaiting = false;

		// Check completion
		for (int32 CompartmentIndex = 0; CompartmentIndex < CompartmentAssemblies.Num(); CompartmentIndex++)
		{
			ProcessCompartmentIfDifferent(
				CompartmentAssemblies[CompartmentIndex],
				CompartmentIndex < Spacecraft->Compartments.Num() ? Spacecraft->Compartments[CompartmentIndex] : FNovaCompartment(),
				FNovaAssemblyCallback::CreateLambda([&](FNovaAssemblyElement& Element, TSoftObjectPtr<UObject> Asset, FNovaAdditionalComponent AdditionalComponent)
					{
						if (Element.Mesh == nullptr)
						{
						}
						else if (Element.Mesh->IsDematerialized())
						{
							UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(Element.Mesh);
							NCHECK(PrimitiveComponent);

							TArray<USceneComponent*> ChildComponents;
							Cast<UPrimitiveComponent>(Element.Mesh)->GetChildrenComponents(false, ChildComponents);
							for (USceneComponent* ChildComponent : ChildComponents)
							{
								ChildComponent->DestroyComponent();
							}
							PrimitiveComponent->DestroyComponent();

							Element.Mesh = nullptr;
						}
						else
						{
							StillWaiting = true;
						}
					}
			));
		}

		// Start the animation
		if (!StillWaiting && !WaitingAssetLoading)
		{
			AssemblyState = ENovaAssemblyState::Moving;
			FVector BuildOffset = FVector::ZeroVector;

			// Compute the initial build offset
			for (int32 CompartmentIndex = 0; CompartmentIndex < CompartmentAssemblies.Num(); CompartmentIndex++)
			{
				const FNovaCompartment& Compartment =
					CompartmentIndex < Spacecraft->Compartments.Num() ? Spacecraft->Compartments[CompartmentIndex] : FNovaCompartment();
				BuildOffset += CompartmentAssemblies[CompartmentIndex]->GetCompartmentLength(Compartment);
			}
			BuildOffset /= 2;

			// Apply individual offsets to compartments
			for (int32 CompartmentIndex = 0; CompartmentIndex < CompartmentAssemblies.Num(); CompartmentIndex++)
			{
				CompartmentAssemblies[CompartmentIndex]->SetRequestedLocation(BuildOffset);
				const FNovaCompartment& Compartment =
					CompartmentIndex < Spacecraft->Compartments.Num() ? Spacecraft->Compartments[CompartmentIndex] : FNovaCompartment();
				BuildOffset -= CompartmentAssemblies[CompartmentIndex]->GetCompartmentLength(Compartment);
			}
		}
	}

	// Wait for the end of the compartment animation
	if (AssemblyState == ENovaAssemblyState::Moving)
	{
		bool StillWaiting = false;
		for (UNovaCompartmentAssembly* Compartment : CompartmentAssemblies)
		{
			if (!Compartment->IsAtRequestedLocation())
			{
				StillWaiting = true;
				break;
			}
		}

		if (!StillWaiting)
		{
			AssemblyState = ENovaAssemblyState::Building;
		}
	}

	// Run the actual building process
	if (AssemblyState == ENovaAssemblyState::Building)
	{
		// Unload unneeded leftover assets
		for (const FSoftObjectPath& Asset : CurrentAssets)
		{
			if (RequestedAssets.Find(Asset) == INDEX_NONE)
			{
				UNovaAssetCatalog::Get()->UnloadAsset(Asset);
			}
		}
		CurrentAssets = RequestedAssets;
		RequestedAssets.Empty();

		// Build the assembly
		int32 ValidIndex = 0;
		TArray<int32> RemovedCompartments;
		for (int32 CompartmentIndex = 0; CompartmentIndex < Spacecraft->Compartments.Num(); CompartmentIndex++)
		{
			UNovaCompartmentAssembly* PreviousCompartment = CompartmentIndex > 0 ? CompartmentAssemblies[CompartmentIndex - 1] : nullptr;
			const FNovaCompartment* PreviousAssembly = CompartmentIndex > 0 ? &Spacecraft->Compartments[CompartmentIndex - 1] : nullptr;
			const FNovaCompartment& Assembly = Spacecraft->Compartments[CompartmentIndex];

			if (Assembly.IsValid())
			{
				CompartmentAssemblies[CompartmentIndex]->BuildCompartment(Assembly, ValidIndex);
				ValidIndex++;
			}
			else
			{
				RemovedCompartments.Add(CompartmentIndex);
			}
		}

		// Destroy removed compartments
		for (int32 Index : RemovedCompartments)
		{
			Spacecraft->Compartments.RemoveAt(Index);
			CompartmentAssemblies[Index]->DestroyComponent();
			CompartmentAssemblies.RemoveAt(Index);
		}

		// Complete the process
		DisplayFilterIndex = FMath::Min(DisplayFilterIndex, Spacecraft->Compartments.Num() - 1);
		AssemblyState = ENovaAssemblyState::Idle;
		UpdateDisplayFilter();
		UpdateBounds();
	}
}

void ANovaSpacecraftAssembly::UpdateDisplayFilter()
{
	for (int32 CompartmentIndex = 0; CompartmentIndex < Spacecraft->Compartments.Num(); CompartmentIndex++)
	{
		ProcessCompartment(CompartmentAssemblies[CompartmentIndex],
			FNovaCompartment(),
			FNovaAssemblyCallback::CreateLambda([=](FNovaAssemblyElement& Element, TSoftObjectPtr<UObject> Asset, FNovaAdditionalComponent AdditionalComponent)
				{
					if (Element.Mesh)
					{
						bool DisplayState = false;
						bool IsFocusingCompartment = DisplayFilterIndex != INDEX_NONE;

						// Process the filter type
						switch (Element.Type)
						{
						case ENovaAssemblyElementType::Module:
							DisplayState = true;
							break;

						case ENovaAssemblyElementType::Structure:
							DisplayState = DisplayFilterType >= ENovaAssemblyDisplayFilter::ModulesStructure;
							break;

						case ENovaAssemblyElementType::Equipment:
							DisplayState = DisplayFilterType >= ENovaAssemblyDisplayFilter::ModulesStructureEquipments;
							break;

						case ENovaAssemblyElementType::Wiring:
							DisplayState = DisplayFilterType >= ENovaAssemblyDisplayFilter::ModulesStructureEquipmentsWiring;
							break;

						case ENovaAssemblyElementType::Hull:
							DisplayState = DisplayFilterType == ENovaAssemblyDisplayFilter::All;
							break;

						default:
							NCHECK(false);
						}

						// Process the filter index : include the current compartment
						if (IsFocusingCompartment && CompartmentIndex != DisplayFilterIndex)
						{
							DisplayState = false;
						}

						if (DisplayState)
						{
							Element.Mesh->Materialize(ImmediateMode);
						}
						else
						{
							Element.Mesh->Dematerialize(ImmediateMode);
						}
					}
				})
		);
	}
}

UNovaCompartmentAssembly* ANovaSpacecraftAssembly::CreateCompartment(const FNovaCompartment& Assembly)
{
	UNovaCompartmentAssembly* NewCompartment = NewObject<UNovaCompartmentAssembly>(this);
	NCHECK(NewCompartment);

	// Setup the compartment
	NewCompartment->Description = Assembly.Description;
	NewCompartment->AttachToComponent(RootComponent, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true));
	NewCompartment->RegisterComponent();
	NewCompartment->SetImmediateMode(ImmediateMode);

	return NewCompartment;
}

void ANovaSpacecraftAssembly::ProcessCompartmentIfDifferent(
	UNovaCompartmentAssembly* CompartmentAssembly,
	const FNovaCompartment& Compartment,
	FNovaAssemblyCallback Callback)
{
	ProcessCompartment(CompartmentAssembly, Compartment,
		FNovaAssemblyCallback::CreateLambda([=](FNovaAssemblyElement& Element, TSoftObjectPtr<UObject> Asset, FNovaAdditionalComponent AdditionalComponent)
		{
			if (Element.Asset != Asset.ToSoftObjectPath() || CompartmentAssembly->Description != Compartment.Description)
			{
				Callback.Execute(Element, Asset, AdditionalComponent);
			}
		})
	);
}

void ANovaSpacecraftAssembly::ProcessCompartment(
	UNovaCompartmentAssembly* CompartmentAssembly,
	const FNovaCompartment& Compartment,
	FNovaAssemblyCallback Callback)
{
	if (CompartmentAssembly->Description)
	{
		CompartmentAssembly->ProcessCompartment(Compartment, Callback);
	}
}

void ANovaSpacecraftAssembly::UpdateBounds()
{
	CurrentOrigin = FVector::ZeroVector;
	CurrentExtent = FVector::ZeroVector;

	// While focusing a compartment, only account for it
	if (DisplayFilterIndex != INDEX_NONE)
	{
		NCHECK(DisplayFilterIndex >= 0 && DisplayFilterIndex < CompartmentAssemblies.Num());

		FBox Bounds(ForceInit);
		ForEachComponent<UPrimitiveComponent>(false, [&](const UPrimitiveComponent* Prim)
			{
				if (Prim->IsRegistered() && Prim->IsVisible()
					&& Prim->IsAttachedTo(CompartmentAssemblies[DisplayFilterIndex])
					&& Prim->Implements<UNovaMeshInterface>())
				{
					Bounds += Prim->Bounds.GetBox();
				}
			});
		Bounds.GetCenterAndExtents(CurrentOrigin, CurrentExtent);
	}

	// In other case, use an estimate to avoid animation bounces
	else
	{
		FVector Unused;
		GetActorBounds(true, Unused, CurrentExtent);
	}
}


#undef LOCTEXT_NAMESPACE
