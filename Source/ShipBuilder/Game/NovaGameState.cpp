// Spaceship Builder - Gwennaël Arbona

#include "NovaGameState.h"

#include "NovaArea.h"
#include "NovaGameMode.h"
#include "NovaGameTypes.h"
#include "NovaAISimulationComponent.h"
#include "NovaAsteroidSimulationComponent.h"
#include "NovaOrbitalSimulationComponent.h"

#include "Actor/NovaActorTools.h"
#include "Player/NovaPlayerState.h"
#include "Player/NovaPlayerController.h"

#include "Spacecraft/NovaSpacecraftPawn.h"
#include "Spacecraft/NovaSpacecraftMovementComponent.h"
#include "Spacecraft/System/NovaSpacecraftSystemInterface.h"

#include "System/NovaAssetManager.h"
#include "System/NovaGameInstance.h"
#include "System/NovaSessionsManager.h"

#include "Nova.h"

#include "Engine/LevelStreaming.h"
#include "Net/UnrealNetwork.h"
#include "Dom/JsonObject.h"
#include "EngineUtils.h"

#define LOCTEXT_NAMESPACE "ANovaGameState"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

ANovaGameState::ANovaGameState()
	: Super()

	, CurrentArea(nullptr)
	, ServerTime(0)
	, ServerTimeDilation(ENovaTimeDilation::Normal)

	, CurrentPriceRotation(1)

	, ClientTime(0)
	, ClientAdditionalTimeDilation(0)
	, IsFastForward(false)
	, TimeSinceLastFastForward(0)

	, TimeSinceEvent(0)
{
	// Setup simulation component
	OrbitalSimulationComponent  = CreateDefaultSubobject<UNovaOrbitalSimulationComponent>(TEXT("OrbitalSimulationComponent"));
	AsteroidSimulationComponent = CreateDefaultSubobject<UNovaAsteroidSimulationComponent>(TEXT("AsteroidSimulationComponent"));
	AISimulationComponent       = CreateDefaultSubobject<UNovaAISimulationComponent>(TEXT("AISimulationComponent"));

	// Settings
	bReplicates = true;
	SetReplicatingMovement(false);
	bAlwaysRelevant               = true;
	PrimaryActorTick.bCanEverTick = true;

	// General defaults
	MinimumTimeCorrectionThreshold = 0.25f;
	MaximumTimeCorrectionThreshold = 10.0f;
	TimeCorrectionFactor           = 1.0f;

	// Fast forward defaults : 2 days per frame in 2h steps
	FastForwardUpdateTime      = 2 * 60;
	FastForwardUpdatesPerFrame = 24;
	FastForwardDelay           = 0.5;

	// Time defaults
	EventNotificationDelay     = 0.5f;
	TrajectoryEarlyRequirement = 5.0;
}

/*----------------------------------------------------
    Loading & saving
----------------------------------------------------*/

struct FNovaGameStateSave
{
	const UNovaArea* CurrentArea;
	double           TimeAsMinutes;
	int32            CurrentPriceRotation;

	TSharedPtr<struct FNovaAIStateSave> AIData;
};

TSharedPtr<FNovaGameStateSave> ANovaGameState::Save() const
{
	NCHECK(GetLocalRole() == ROLE_Authority);

	TSharedPtr<FNovaGameStateSave> SaveData = MakeShared<FNovaGameStateSave>();

	// Save general state
	SaveData->CurrentArea          = GetCurrentArea();
	SaveData->TimeAsMinutes        = GetCurrentTime().AsMinutes();
	SaveData->CurrentPriceRotation = CurrentPriceRotation;

	// Save AI
	SaveData->AIData = AISimulationComponent->Save();

	// Ensure consistency
	NCHECK(SaveData->TimeAsMinutes > 0);
	NCHECK(IsValid(SaveData->CurrentArea));

	return SaveData;
}

void ANovaGameState::Load(TSharedPtr<FNovaGameStateSave> SaveData)
{
	NCHECK(GetLocalRole() == ROLE_Authority);

	NLOG("ANovaGameState::Load");

	// Ensure consistency
	NCHECK(SaveData != nullptr);
	NCHECK(SaveData->TimeAsMinutes >= 0);
	NCHECK(IsValid(SaveData->CurrentArea));

	// Save general state
	SetCurrentArea(SaveData->CurrentArea);
	ServerTime           = SaveData->TimeAsMinutes;
	CurrentPriceRotation = SaveData->CurrentPriceRotation;

	// Load AI
	AISimulationComponent->Load(SaveData->AIData);
}

void ANovaGameState::SerializeJson(TSharedPtr<FNovaGameStateSave>& SaveData, TSharedPtr<FJsonObject>& JsonData, ENovaSerialize Direction)
{
	// Writing to save
	if (Direction == ENovaSerialize::DataToJson)
	{
		JsonData = MakeShared<FJsonObject>();

		// General state
		UNovaAssetDescription::SaveAsset(JsonData, "CurrentArea", SaveData->CurrentArea);
		JsonData->SetNumberField("TimeAsMinutes", SaveData->TimeAsMinutes);
		JsonData->SetNumberField("CurrentPriceRotation", SaveData->CurrentPriceRotation);

		// AI
		TSharedPtr<FJsonObject> AIJsonData = MakeShared<FJsonObject>();
		UNovaAISimulationComponent::SerializeJson(SaveData->AIData, AIJsonData, ENovaSerialize::DataToJson);
		JsonData->SetObjectField("AI", AIJsonData);
	}

	// Reading from save
	else
	{
		SaveData = MakeShared<FNovaGameStateSave>();

		// Area
		SaveData->CurrentArea = UNovaAssetDescription::LoadAsset<UNovaArea>(JsonData, "CurrentArea");
		if (!IsValid(SaveData->CurrentArea))
		{
			SaveData->CurrentArea = UNovaAssetManager::Get()->GetDefaultAsset<UNovaArea>();
		}

		// Time
		double Time;
		if (JsonData->TryGetNumberField("TimeAsMinutes", Time))
		{
			SaveData->TimeAsMinutes = Time;
		}

		// Price rotation
		int32 PriceRotation;
		if (JsonData->TryGetNumberField("CurrentPriceRotation", PriceRotation))
		{
			SaveData->CurrentPriceRotation = PriceRotation;
		}

		// AI
		TSharedPtr<FJsonObject> AIJsonData =
			JsonData->HasTypedField<EJson::Object>("AI") ? JsonData->GetObjectField("AI") : MakeShared<FJsonObject>();
		UNovaAISimulationComponent::SerializeJson(SaveData->AIData, AIJsonData, ENovaSerialize::JsonToData);
	}
}

/*----------------------------------------------------
    General game state
----------------------------------------------------*/

void ANovaGameState::BeginPlay()
{
	Super::BeginPlay();

	UNovaAssetManager* AssetManager = GetGameInstance<UNovaGameInstance>()->GetAssetManager();
	NCHECK(AssetManager);

	// Startup the simulation components
	AsteroidSimulationComponent->Initialize(AssetManager->GetDefaultAsset<UNovaAsteroidConfiguration>());
}

void ANovaGameState::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Process fast forward simulation
	if (IsFastForward)
	{
		int64     Cycles         = FPlatformTime::Cycles64();
		FNovaTime InitialTime    = GetCurrentTime();
		TimeSinceLastFastForward = 0;

		// Run FastForwardUpdatesPerFrame loops of world updates
		for (int32 Index = 0; Index < FastForwardUpdatesPerFrame; Index++)
		{
			bool ContinueProcessing = ProcessGameSimulation(FNovaTime::FromMinutes(FastForwardUpdateTime));
			if (!ContinueProcessing)
			{
				NLOG("ANovaGameState::ProcessTime : fast-forward stopping at %.2f", ServerTime);
				IsFastForward = false;
				break;
			}
		}

		// Check the time jump
		FNovaTime FastForwardDeltaTime = GetCurrentTime() - InitialTime;
		if (FastForwardDeltaTime > FNovaTime::FromMinutes(1))
		{
			TimeJumpEvents.Add(FastForwardDeltaTime);
			TimeSinceEvent = 0;
		}

		NLOG("ANovaGameState::Tick : processed fast-forward frame in %.2fms",
			FPlatformTime::ToMilliseconds(FPlatformTime::Cycles64() - Cycles));
	}

	// Process real-time simulation
	else
	{
		ProcessGameSimulation(FNovaTime::FromSeconds(static_cast<double>(DeltaTime)));

		TimeSinceLastFastForward += DeltaTime;
	}

	// Update event notification
	ProcessPlayerEvents(DeltaTime);

	// Update sessions
	UNovaSessionsManager* SessionsManager = GetGameInstance<UNovaGameInstance>()->GetSessionsManager();
	SessionsManager->SetSessionAdvertised(IsJoinable());
}

void ANovaGameState::SetCurrentArea(const UNovaArea* Area)
{
	NCHECK(GetLocalRole() == ROLE_Authority);

	CurrentArea = Area;

	AreaChangeEvents.Add(CurrentArea);
	TimeSinceEvent = 0;
}

void ANovaGameState::RotatePrices()
{
	CurrentPriceRotation++;

	NLOG("ANovaGameState::RotatePrices (now %d)", CurrentPriceRotation);
}

FName ANovaGameState::GetCurrentLevelName() const
{
	return CurrentArea ? CurrentArea->LevelName : NAME_None;
}

bool ANovaGameState::IsLevelStreamingComplete() const
{
	for (const ULevelStreaming* Level : GetWorld()->GetStreamingLevels())
	{
		if (Level->IsStreamingStatePending())
		{
			return false;
		}
		else if (Level->IsLevelLoaded())
		{
			FString LoadedLevelName = Level->GetWorldAssetPackageFName().ToString();
			return LoadedLevelName.EndsWith(GetCurrentLevelName().ToString());
		}
	}

	return true;
}

bool ANovaGameState::IsJoinable(FText* Help) const
{
	bool AllSpacecraftDocked = AreAllSpacecraftDocked();

	if (GetWorld()->WorldType == EWorldType::PIE)
	{
		return true;
	}
	else if (!AllSpacecraftDocked && Help)
	{
		*Help = LOCTEXT("AllSpacecraftNotDocked", "All players need to be docked to allow joining");
	}

	return AllSpacecraftDocked;
}

/*----------------------------------------------------
    Resources
----------------------------------------------------*/

bool ANovaGameState::IsResourceSold(const UNovaResource* Asset, const class UNovaArea* Area) const
{
	const UNovaArea* TargetArea = IsValid(Area) ? Area : CurrentArea;

	if (IsValid(TargetArea))
	{
		for (const FNovaResourceTrade& Trade : TargetArea->ResourceTradeMetadata)
		{
			if (Trade.Resource == Asset)
			{
				return Trade.ForSale;
			}
		}
	}

	return false;
}

TArray<const UNovaResource*> ANovaGameState::GetResourcesSold() const
{
	TArray<const UNovaResource*> Result;

	if (IsValid(CurrentArea))
	{
		for (const FNovaResourceTrade& Trade : CurrentArea->ResourceTradeMetadata)
		{
			if (Trade.ForSale)
			{
				Result.Add(Trade.Resource);
			}
		}
	}

	return Result;
}

ENovaPriceModifier ANovaGameState::GetCurrentPriceModifier(const UNovaTradableAssetDescription* Asset, const UNovaArea* Area) const
{
	NCHECK(Area);

	// Rotate pricing without affecting the current area since the player came here for a reason
	auto RotatePrice = [Area, this](ENovaPriceModifier Input)
	{
		int32 PriceRotation = (Area == GetCurrentArea() ? CurrentPriceRotation - 1 : CurrentPriceRotation) % 3;

		return Input;
	};

	// Find the relevant trade metadata if any
	if (IsValid(CurrentArea))
	{
		for (const FNovaResourceTrade& Trade : CurrentArea->ResourceTradeMetadata)
		{
			if (Trade.Resource == Asset)
			{
				return RotatePrice(Trade.PriceModifier);
			}
		}
	}

	// Default to average price
	return RotatePrice(ENovaPriceModifier::Average);
}

FNovaCredits ANovaGameState::GetCurrentPrice(
	const UNovaTradableAssetDescription* Asset, const UNovaArea* Area, bool SpacecraftPartForSale) const
{
	NCHECK(IsValid(Asset));
	float Multiplier = 1.0f;

	// Define price modifiers
	auto GetPriceModifierValue = [](ENovaPriceModifier Modifier) -> double
	{
		switch (Modifier)
		{
			case ENovaPriceModifier::Cheap:
				return 0.75f;
			case ENovaPriceModifier::BelowAverage:
				return 0.9f;
			case ENovaPriceModifier::Average:
				return 1.0f;
			case ENovaPriceModifier::AboveAverage:
				return 1.1f;
			case ENovaPriceModifier::Expensive:
				return 1.25f;
		}

		return 0.0f;
	};

	// Find out the current modifier for this transaction
	Multiplier *= GetPriceModifierValue(GetCurrentPriceModifier(Asset, Area));

	// Non-resource assets have a large depreciation value when re-sold
	if (!Asset->IsA<UNovaResource>() && SpacecraftPartForSale)
	{
		Multiplier *= ENovaConstants::ResaleDepreciation;
	}

	return Multiplier * FNovaCredits(Asset->BasePrice);
}

/*----------------------------------------------------
    Spacecraft management
----------------------------------------------------*/

void ANovaGameState::UpdatePlayerSpacecraft(const FNovaSpacecraft& Spacecraft, bool MergeWithPlayer)
{
	NCHECK(GetLocalRole() == ROLE_Authority);

	NLOG("ANovaGameState::UpdatePlayerSpacecraft");

	PlayerSpacecraftIdentifiers.AddUnique(Spacecraft.Identifier);

	bool IsNew = SpacecraftDatabase.Add(Spacecraft);
	if (IsNew)
	{
		// Attempt orbit merging for player spacecraft joining the game
		bool WasMergedWithPlayer = false;
		if (MergeWithPlayer)
		{
			ANovaGameState* GameState = GetWorld()->GetGameState<ANovaGameState>();
			NCHECK(GameState);
			const FNovaOrbit* CurrentOrbit = OrbitalSimulationComponent->GetPlayerOrbit();

			if (CurrentOrbit)
			{
				OrbitalSimulationComponent->MergeOrbit(GameState->GetPlayerSpacecraftIdentifiers(), *CurrentOrbit);
				WasMergedWithPlayer = true;
			}
		}

		// Load a default
		if (!WasMergedWithPlayer)
		{
			NCHECK(IsValid(CurrentArea));
			OrbitalSimulationComponent->SetOrbit({Spacecraft.Identifier}, OrbitalSimulationComponent->GetAreaOrbit(GetCurrentArea()));
		}
	}
}

void ANovaGameState::UpdateSpacecraft(const FNovaSpacecraft& Spacecraft, const FNovaOrbit* Orbit)
{
	NCHECK(GetLocalRole() == ROLE_Authority);

	NLOG("ANovaGameState::UpdateSpacecraft");

	bool IsNew = SpacecraftDatabase.Add(Spacecraft);
	if (IsNew && Orbit != nullptr && Orbit->IsValid())
	{
		OrbitalSimulationComponent->SetOrbit({Spacecraft.Identifier}, *Orbit);
	}
}

bool ANovaGameState::IsAnySpacecraftDocked() const
{
	for (const ANovaSpacecraftPawn* SpacecraftPawn : TActorRange<ANovaSpacecraftPawn>(GetWorld()))
	{
		if (SpacecraftPawn->IsDocked())
		{
			return true;
		}
	}

	return false;
}

bool ANovaGameState::AreAllSpacecraftDocked() const
{
	for (const ANovaSpacecraftPawn* SpacecraftPawn : TActorRange<ANovaSpacecraftPawn>(GetWorld()))
	{
		if (!SpacecraftPawn->IsDocked())
		{
			return false;
		}
	}
	return true;
}

UActorComponent* ANovaGameState::GetSpacecraftSystem(
	const struct FNovaSpacecraft* Spacecraft, const TSubclassOf<UActorComponent> ComponentClass) const
{
	NCHECK(Spacecraft);

	// TODO : not super clean, could warrant a physical spacecraft table

	for (const ANovaSpacecraftPawn* SpacecraftPawn : TActorRange<ANovaSpacecraftPawn>(GetWorld()))
	{
		if (::IsValid(SpacecraftPawn) && SpacecraftPawn->GetSpacecraftIdentifier() == Spacecraft->Identifier)
		{
			return SpacecraftPawn->FindComponentByClass(ComponentClass);
		}
	}

	return nullptr;
}

/*----------------------------------------------------
    Time management
----------------------------------------------------*/

FNovaTime ANovaGameState::GetCurrentTime() const
{
	if (GetLocalRole() == ROLE_Authority)
	{
		return FNovaTime::FromMinutes(ServerTime);
	}
	else
	{
		return FNovaTime::FromMinutes(ClientTime);
	}
}

FNovaTime ANovaGameState::GetTimeLeftUntilEvent() const
{
	const ANovaGameMode* GameMode      = GetWorld()->GetAuthGameMode<ANovaGameMode>();
	FNovaTime            RemainingTime = OrbitalSimulationComponent->GetTimeLeftUntilPlayerManeuver(GameMode->GetManeuverWarnTime());

	const FNovaTrajectory* PlayerTrajectory = OrbitalSimulationComponent->GetPlayerTrajectory();
	if (PlayerTrajectory)
	{
		const FNovaTime TimeBeforeArrival = PlayerTrajectory->GetArrivalTime() - GetCurrentTime() - GameMode->GetArrivalWarningTime();

		RemainingTime = FMath::Min(RemainingTime, TimeBeforeArrival);
	}

	return RemainingTime;
}

void ANovaGameState::FastForward()
{
	NLOG("ANovaGameState::FastForward");
	SetTimeDilation(ENovaTimeDilation::Normal);
	IsFastForward = true;
}

bool ANovaGameState::CanFastForward(FText* AbortReason) const
{
	if (GetLocalRole() == ROLE_Authority)
	{
		// No event upcoming
		if (GetTimeLeftUntilEvent() > FNovaTime::FromDays(ENovaConstants::MaxTrajectoryDurationDays))
		{
			if (AbortReason)
			{
				*AbortReason = LOCTEXT("NoEvent", "No upcoming event");
			}
			return false;
		}

		// Maneuvering
		const FNovaTrajectory* PlayerTrajectory = OrbitalSimulationComponent->GetPlayerTrajectory();
		if (PlayerTrajectory && PlayerTrajectory->GetManeuver(GetCurrentTime()))
		{
			if (AbortReason)
			{
				*AbortReason = LOCTEXT("Maneuvering", "A maneuver is ongoing");
			}
			return false;
		}

		// Check trajectory issues
		return CheckTrajectoryAbort(AbortReason) == ENovaTrajectoryAction::Continue;
	}
	else
	{
		return false;
	}
}

void ANovaGameState::SetTimeDilation(ENovaTimeDilation Dilation)
{
	NCHECK(GetLocalRole() == ROLE_Authority);
	NCHECK(Dilation >= ENovaTimeDilation::Normal && Dilation <= ENovaTimeDilation::Level3);

	ServerTimeDilation = Dilation;
}

bool ANovaGameState::CanDilateTime(ENovaTimeDilation Dilation) const
{
	return GetLocalRole() == ROLE_Authority;
}

/*----------------------------------------------------
    Internals
----------------------------------------------------*/

bool ANovaGameState::ProcessGameSimulation(FNovaTime DeltaTime)
{
	// Update spacecraft
	SpacecraftDatabase.UpdateCache();
	for (FNovaSpacecraft& Spacecraft : SpacecraftDatabase.Get())
	{
		Spacecraft.UpdatePropulsionMetrics();
	}

	// Update the time with the base delta time that will be affected by time dilation
	FNovaTime InitialTime        = GetCurrentTime();
	bool      ContinueProcessing = ProcessGameTime(DeltaTime);

	// Update the orbital simulation
	OrbitalSimulationComponent->UpdateSimulation();

	// Abort trajectories when a player didn't commit in time
	if (GetLocalRole() == ROLE_Authority)
	{
		ProcessTrajectoryAbort();
	}

	// Update spacecraft systems
	if (GetLocalRole() == ROLE_Authority)
	{
		for (ANovaSpacecraftPawn* Pawn : TActorRange<ANovaSpacecraftPawn>(GetWorld()))
		{
			if (Pawn->GetPlayerState() != nullptr)
			{
				TArray<UActorComponent*> Components = Pawn->GetComponentsByInterface(UNovaSpacecraftSystemInterface::StaticClass());
				for (UActorComponent* Component : Components)
				{
					INovaSpacecraftSystemInterface* System = Cast<INovaSpacecraftSystemInterface>(Component);
					NCHECK(System);
					System->Update(InitialTime, GetCurrentTime());
				}
			}
		}
	}

	return ContinueProcessing;
}

bool ANovaGameState::ProcessGameTime(FNovaTime DeltaTime)
{
	bool         ContinueProcessing = true;
	const double TimeDilation       = GetCurrentTimeDilationValue();

	// Under fast forward, stop on events
	if (IsFastForward && GetLocalRole() == ROLE_Authority)
	{
		const ANovaGameMode* GameMode = GetWorld()->GetAuthGameMode<ANovaGameMode>();

		// Check for upcoming events
		FNovaTime MaxAllowedDeltaTime = OrbitalSimulationComponent->GetTimeLeftUntilPlayerManeuver(GameMode->GetManeuverWarnTime());
		const FNovaTrajectory* PlayerTrajectory = OrbitalSimulationComponent->GetPlayerTrajectory();
		if (PlayerTrajectory)
		{
			const FNovaTime TimeBeforeArrival = PlayerTrajectory->GetArrivalTime() - GetCurrentTime() - GameMode->GetArrivalWarningTime();

			MaxAllowedDeltaTime = FMath::Min(MaxAllowedDeltaTime, TimeBeforeArrival);
		}

		NCHECK(TimeDilation == 1.0);
		NCHECK(MaxAllowedDeltaTime > FNovaTime());

		// Break simulation if we have an event soon
		if (DeltaTime > MaxAllowedDeltaTime)
		{
			NLOG("ANovaGameState::ProcessGameTime : stopping processing");

			DeltaTime          = MaxAllowedDeltaTime;
			ContinueProcessing = false;
		}
	}

	// Update the time
	const double DilatedDeltaTime = TimeDilation * DeltaTime.AsMinutes();
	if (GetLocalRole() == ROLE_Authority)
	{
		ServerTime += DilatedDeltaTime;
	}
	else
	{
		ClientTime += DilatedDeltaTime * ClientAdditionalTimeDilation;
	}

	return ContinueProcessing;
}

void ANovaGameState::ProcessPlayerEvents(float DeltaTime)
{
	FText PrimaryText, SecondaryText;

	TimeSinceEvent += DeltaTime;

	if (TimeSinceEvent > EventNotificationDelay)
	{
		ENovaNotificationType NotificationType = ENovaNotificationType::Info;

		// Handle area changes as the primary information
		if (AreaChangeEvents.Num())
		{
			PrimaryText      = AreaChangeEvents[AreaChangeEvents.Num() - 1]->Name;
			NotificationType = ENovaNotificationType::World;
		}

		// Handle time skips as the secondary information
		if (TimeJumpEvents.Num())
		{
			FText& Text = PrimaryText.IsEmpty() ? PrimaryText : SecondaryText;

			if (PrimaryText.IsEmpty())
			{
				NotificationType = ENovaNotificationType::Time;
			}

			Text = FText::FormatNamed(LOCTEXT("SharedTransitionTimeFormat", "{duration} have passed"), TEXT("duration"),
				GetDurationText(TimeJumpEvents[TimeJumpEvents.Num() - 1], 1));
		}

		if (!PrimaryText.IsEmpty())
		{
			ANovaPlayerController* PC = Cast<ANovaPlayerController>(GetGameInstance()->GetFirstLocalPlayerController());
			if (IsValid(PC) && PC->IsLocalController())
			{
				PC->Notify(PrimaryText, SecondaryText, NotificationType);
			}
		}

		AreaChangeEvents.Empty();
		TimeJumpEvents.Empty();
	}
}

void ANovaGameState::ProcessTrajectoryAbort()
{
	const FNovaTrajectory* PlayerTrajectory = OrbitalSimulationComponent->GetPlayerTrajectory();
	if (PlayerTrajectory)
	{
		// Check all spacecraft for issues
		FText                 AbortReason;
		ENovaTrajectoryAction TrajectoryState = CheckTrajectoryAbort(&AbortReason);

		// Check whether the trajectory is less than a few seconds away from starting
		bool IsTrajectoryStarted =
			PlayerTrajectory && PlayerTrajectory->GetManeuver(GetCurrentTime()) == nullptr &&
			(PlayerTrajectory->GetNextManeuverStartTime(GetCurrentTime()) - GetCurrentTime()).AsSeconds() < TrajectoryEarlyRequirement;

		// Invalidate the trajectory if a player doesn't match conditions
		if (TrajectoryState == ENovaTrajectoryAction::AbortImmediately ||
			(TrajectoryState == ENovaTrajectoryAction::AbortIfStarted && IsTrajectoryStarted))
		{
			NLOG("ANovaGameState::ProcessTrajectoryAbort : aborting trajectory");

			OrbitalSimulationComponent->AbortTrajectory(GetPlayerSpacecraftIdentifiers());
			ANovaPlayerController* PC = Cast<ANovaPlayerController>(GetGameInstance()->GetFirstLocalPlayerController());
			PC->Notify(LOCTEXT("TrajectoryAborted", "Trajectory aborted"), AbortReason, ENovaNotificationType::Error);

			return;
		}
	}
}

ENovaTrajectoryAction ANovaGameState::CheckTrajectoryAbort(FText* AbortReason) const
{
	for (const ANovaSpacecraftPawn* Pawn : TActorRange<ANovaSpacecraftPawn>(GetWorld()))
	{
		if (Pawn->GetPlayerState())
		{
			// Docking or undocking
			if (Pawn->GetSpacecraftMovement()->GetState() == ENovaMovementState::Docking ||
				Pawn->GetSpacecraftMovement()->GetState() == ENovaMovementState::Docked)
			{
				if (AbortReason)
				{
					*AbortReason =
						FText::FormatNamed(LOCTEXT("SpacecraftDocking", "{spacecraft}|plural(one=The,other=A) spacecraft is docking"),
							TEXT("spacecraft"), PlayerArray.Num());
				}

				return ENovaTrajectoryAction::AbortImmediately;
			}

			// Maneuvers not cleared by player
			else if (!Pawn->GetSpacecraftMovement()->CanManeuver())
			{
				if (AbortReason)
				{
					*AbortReason = FText::FormatNamed(
						LOCTEXT("SpacecraftNotManeuvering", "{spacecraft}|plural(one=The,other=A) spacecraft isn't correctly oriented"),
						TEXT("spacecraft"), PlayerArray.Num());
				}

				return ENovaTrajectoryAction::AbortIfStarted;
			}
		}
	}

	return ENovaTrajectoryAction::Continue;
}

void ANovaGameState::OnServerTimeReplicated()
{
	const APlayerController* PC = GetGameInstance()->GetFirstLocalPlayerController();
	NCHECK(IsValid(PC) && PC->IsLocalController());

	// Evaluate the current server time
	const double PingSeconds      = UNovaActorTools::GetPlayerLatency(PC);
	const double RealServerTime   = ServerTime + PingSeconds / 60.0;
	const double TimeDeltaSeconds = (RealServerTime - ClientTime) * 60.0 / GetCurrentTimeDilationValue();

	// We can never go back in time
	NCHECK(TimeDeltaSeconds > -MaximumTimeCorrectionThreshold);

	// Hard correct if the change is large
	if (TimeDeltaSeconds > MaximumTimeCorrectionThreshold)
	{
		NLOG("ANovaGameState::OnServerTimeReplicated : time jump from %.2f to %.2f", ClientTime, RealServerTime);

		TimeJumpEvents.Add(FNovaTime::FromMinutes(RealServerTime - ClientTime));
		TimeSinceEvent = 0;

		ClientTime                   = RealServerTime;
		ClientAdditionalTimeDilation = 1.0;
	}

	// Smooth correct if it isn't
	else
	{
		const float TimeDeltaRatio = FMath::Clamp(
			(TimeDeltaSeconds - MinimumTimeCorrectionThreshold) / (MaximumTimeCorrectionThreshold - MinimumTimeCorrectionThreshold), 0.0,
			1.0);

		ClientAdditionalTimeDilation = 1.0 + TimeDeltaRatio * TimeCorrectionFactor * FMath::Sign(TimeDeltaSeconds);
	}
}

void ANovaGameState::OnCurrentAreaReplicated()
{
	NLOG("ANovaGameState::OnCurrentAreaReplicated");

	AreaChangeEvents.Add(CurrentArea);
	TimeSinceEvent = 0;
}

void ANovaGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANovaGameState, CurrentArea);

	DOREPLIFETIME(ANovaGameState, SpacecraftDatabase);
	DOREPLIFETIME(ANovaGameState, PlayerSpacecraftIdentifiers);
	DOREPLIFETIME(ANovaGameState, ServerTime);
	DOREPLIFETIME(ANovaGameState, ServerTimeDilation);
}

#undef LOCTEXT_NAMESPACE
