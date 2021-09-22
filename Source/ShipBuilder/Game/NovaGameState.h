// Spaceship Builder - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"

#include "NovaOrbitalSimulationDatabases.h"

#include "NovaGameState.generated.h"

/** Time dilation settings */
UENUM()
enum class ENovaTimeDilation : uint8
{
	Normal,
	Level1,
	Level2,
	Level3
};

/** Trajectory error levels */
enum class ENovaTrajectoryAction : uint8
{
	Continue,
	AbortIfStarted,
	AbortImmediately
};

/** Replicated game state class */
UCLASS(ClassGroup = (Nova))
class ANovaGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	ANovaGameState();

	/*----------------------------------------------------
	    Loading & saving
	----------------------------------------------------*/

	TSharedPtr<struct FNovaGameStateSave> Save() const;

	void Load(TSharedPtr<struct FNovaGameStateSave> SaveData);

	static void SerializeJson(
		TSharedPtr<struct FNovaGameStateSave>& SaveData, TSharedPtr<class FJsonObject>& JsonData, ENovaSerialize Direction);

	/*----------------------------------------------------
	    General game state
	----------------------------------------------------*/

	virtual void BeginPlay() override;

	virtual void Tick(float DeltaTime) override;

	/** Set the current area to use */
	void SetCurrentArea(const class UNovaArea* Area);

	/** Get the current area we are at */
	const class UNovaArea* GetCurrentArea() const
	{
		return CurrentArea;
	}

	/** Get the current sub-level name to use */
	FName GetCurrentLevelName() const;

	/** Check if the game state is ready */
	bool IsReady()
	{
		return GetCurrentArea() != nullptr && IsLevelStreamingComplete() && TimeSinceLastFastForward > FastForwardDelay;
	}

	/** Check if loading is currently occurring */
	bool IsLevelStreamingComplete() const;

	/** Check whether the game can be joined */
	bool IsJoinable(FText* Help = nullptr) const;

	/*----------------------------------------------------
	    Resources
	----------------------------------------------------*/

	/** Is a particular resource sold in this area */
	bool IsResourceSold(const class UNovaResource* Asset, const class UNovaArea* Area = nullptr) const;

	/** Get resources sold in this area */
	TArray<const class UNovaResource*> GetResourcesSold() const;

	/** Get the current price modifier of an asset */
	ENovaPriceModifier GetCurrentPriceModifier(const class UNovaTradableAssetDescription* Asset) const;

	/** Get the current price of an asset */
	FNovaCredits GetCurrentPrice(const class UNovaTradableAssetDescription* Asset, bool SpacecraftPartForSale = false) const;

	/*----------------------------------------------------
	    Spacecraft management
	----------------------------------------------------*/

	/** Register or update a player spacecraft */
	void UpdatePlayerSpacecraft(const FNovaSpacecraft& Spacecraft, bool MergeWithPlayer);

	/** Register or update a spacecraft */
	void UpdateSpacecraft(const FNovaSpacecraft& Spacecraft, const FNovaOrbit* Orbit = nullptr);

	/** Remove a spacecraft */
	void RemoveSpacecraft(const FGuid& Identifier)
	{
		PlayerSpacecraftIdentifiers.Remove(Identifier);
		SpacecraftDatabase.Remove(Identifier);
	}

	/** Return a pointer for a spacecraft by identifier */
	const FNovaSpacecraft* GetSpacecraft(const FGuid& Identifier) const
	{
		return SpacecraftDatabase.Get(Identifier);
	}

	/** Return the identifier of one of the player spacecraft */
	FGuid GetPlayerSpacecraftIdentifier() const
	{
		return PlayerSpacecraftIdentifiers.Num() ? PlayerSpacecraftIdentifiers[0] : FGuid();
	}

	/** Return the identifiers of all of the player spacecraft */
	TArray<FGuid> GetPlayerSpacecraftIdentifiers() const
	{
		return PlayerSpacecraftIdentifiers;
	}

	/** Check whether any spacecraft is docked */
	bool IsAnySpacecraftDocked() const;

	/** Check whether all spacecraft are docked */
	bool AreAllSpacecraftDocked() const;

	/** Get a component of the linked spacecraft pawn if any */
	template <class T>
	T* GetSpacecraftSystem(const struct FNovaSpacecraft* Spacecraft) const
	{
		return (T*) GetSpacecraftSystem(Spacecraft, T::StaticClass());
	}

	/** Get a component of the linked spacecraft pawn if any */
	UActorComponent* GetSpacecraftSystem(const struct FNovaSpacecraft* Spacecraft, TSubclassOf<UActorComponent> ComponentClass) const;

	/*----------------------------------------------------
	    Time management
	----------------------------------------------------*/

	/** Get the current game time */
	FNovaTime GetCurrentTime() const;

	/** Get the time left until the next event */
	FNovaTime GetTimeLeftUntilEvent() const;

	/** Simulate the world at full speed until an event */
	void FastForward();

	/** Check if we can skip time */
	bool CanFastForward(FText* AbortReason = nullptr) const;

	/** Check if we are in a time skip */
	bool IsInFastForward() const
	{
		return IsFastForward;
	}

	/** Get the current time dilation factor */
	void SetTimeDilation(ENovaTimeDilation Dilation);

	/** Get time dilation values */
	static float GetTimeDilationValue(ENovaTimeDilation Dilation)
	{
		constexpr float TimeDilationValues[] = {
			1,       // Normal : 1s = 1s
			60,      // Level1 : 1s = 1m
			1200,    // Level2 : 1s = 20m
			7200,    // Level3 : 1s = 2h
		};

		return TimeDilationValues[static_cast<int32>(Dilation)];
	}

	/** Get the current time dilation */
	ENovaTimeDilation GetCurrentTimeDilation() const
	{
		return ServerTimeDilation;
	}

	/** Get the current time dilation */
	double GetCurrentTimeDilationValue() const
	{
		return GetTimeDilationValue(ServerTimeDilation);
	}

	/** Check if we can dilate time */
	bool CanDilateTime(ENovaTimeDilation Dilation) const;

	/*----------------------------------------------------
	    Internals
	----------------------------------------------------*/

protected:
	/** Run all game processes, returns true if simulation can continue */
	bool ProcessGameSimulation(FNovaTime DeltaTime);

	/** Process time */
	bool ProcessGameTime(FNovaTime DeltaTime);

	/** Notify events to the player*/
	void ProcessPlayerEvents(float DeltaTime);

	/** Automatically abort failed trajectories */
	void ProcessTrajectoryAbort();

	/** Check if all player spacecraft can currently maneuver */
	ENovaTrajectoryAction CheckTrajectoryAbort(FText* AbortReason = nullptr) const;

	/** Server replication event for time reconciliation */
	UFUNCTION()
	void OnServerTimeReplicated();

	/** Server replication event for notifications */
	UFUNCTION()
	void OnCurrentAreaReplicated();

	/*----------------------------------------------------
	    Properties
	----------------------------------------------------*/

protected:
	// Threshold in seconds above which the client time starts compensating
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	float MinimumTimeCorrectionThreshold;

	// Threshold in seconds above which the client time is at maximum compensation
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	float MaximumTimeCorrectionThreshold;

	// Maximum time dilation applied to compensate time
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	float TimeCorrectionFactor;

	// Time between simulation updates during fast forward in minutes
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	int32 FastForwardUpdateTime;

	// Number of update steps to run per frame under fast forward
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	int32 FastForwardUpdatesPerFrame;

	// Time in seconds to wait in loading after a fast forward
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	float FastForwardDelay;

	// Time to wait in seconds after an event before notifying it with possible others in between
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	float EventNotificationDelay;

	// Time in seconds before a trajectory starts at which trajectories need to be committed
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	float TrajectoryEarlyRequirement;

	/*----------------------------------------------------
	    Components
	----------------------------------------------------*/

protected:
	// Global orbital simulation component
	UPROPERTY(Category = Nova, VisibleDefaultsOnly, BlueprintReadOnly)
	class UNovaOrbitalSimulationComponent* OrbitalSimulationComponent;

	// Asteroid simulation component
	UPROPERTY(Category = Nova, VisibleDefaultsOnly, BlueprintReadOnly)
	class UNovaAsteroidSimulationComponent* AsteroidSimulationComponent;

	// AI simulation component
	UPROPERTY(Category = Nova, VisibleDefaultsOnly, BlueprintReadOnly)
	class UNovaAISimulationComponent* AISimulationComponent;

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

private:
	// Current level-based area
	UPROPERTY(ReplicatedUsing = OnCurrentAreaReplicated)
	const class UNovaArea* CurrentArea;

	// Replicated spacecraft database
	UPROPERTY(Replicated)
	FNovaSpacecraftDatabase SpacecraftDatabase;

	// Spacecraft identifiers
	UPROPERTY(Replicated)
	TArray<FGuid> PlayerSpacecraftIdentifiers;

	// Replicated world time value in minutes
	UPROPERTY(ReplicatedUsing = OnServerTimeReplicated)
	double ServerTime;

	// Replicated world time dilation
	UPROPERTY(Replicated)
	ENovaTimeDilation ServerTimeDilation;

	// Time processing state
	double ClientTime;
	double ClientAdditionalTimeDilation;
	bool   IsFastForward;
	float  TimeSinceLastFastForward;

	// Event observation system
	float                          TimeSinceEvent;
	TArray<FNovaTime>              TimeJumpEvents;
	TArray<const class UNovaArea*> AreaChangeEvents;

public:
	/*----------------------------------------------------
	    Getters
	----------------------------------------------------*/

	/** Return the orbital simulation component */
	class UNovaOrbitalSimulationComponent* GetOrbitalSimulation() const
	{
		return OrbitalSimulationComponent;
	}

	/** Return the asteroid simulation component */
	class UNovaAsteroidSimulationComponent* GetAsteroidSimulation() const
	{
		return AsteroidSimulationComponent;
	}

	/** Return the AI simulation component */
	class UNovaAISimulationComponent* GetAISimulation() const
	{
		return AISimulationComponent;
	}
};
