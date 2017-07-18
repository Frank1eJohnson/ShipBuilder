// Nova project - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "NovaGameTypes.h"
#include "Nova/Spacecraft/NovaSpacecraft.h"

#include "NovaGameWorld.generated.h"

/** Spacecraft database entry */
struct FNovaSpacecraftDatabaseEntry
{
	TSharedPtr<FNovaSpacecraft> Spacecraft;
	bool                        IsPlayer;
};

/** World manager class */
UCLASS(ClassGroup = (Nova))
class ANovaGameWorld : public AActor
{
	GENERATED_BODY()

public:
	ANovaGameWorld();

	/*----------------------------------------------------
	    Loading & saving
	----------------------------------------------------*/

	TSharedPtr<struct FNovaWorldSave> Save() const;

	void Load(TSharedPtr<struct FNovaWorldSave> SaveData);

	static void SerializeJson(
		TSharedPtr<struct FNovaWorldSave>& SaveData, TSharedPtr<class FJsonObject>& JsonData, ENovaSerialize Direction);

	/*----------------------------------------------------
	    Gameplay
	----------------------------------------------------*/

public:
	/** Register a new AI spacecraft */
	void AddAISpacecraft(const FNovaSpacecraft Spacecraft);

	/** Register a new player spacecraft */
	void AddPlayerSpacecraft(const FNovaSpacecraft Spacecraft);

	/** Return a weak pointer for a spacecraft by identifier */
	TSharedPtr<FNovaSpacecraft> GetSpacecraft(FGuid Identifier);

	/** Get the spacecraft database */
	TMap<FGuid, FNovaSpacecraftDatabaseEntry>& GetSpacecraftDatabase()
	{
		return SpacecraftDatabase;
	}

	/** Return the orbital simulation class */
	class UNovaOrbitalSimulationComponent* GetOrbitalSimulation() const
	{
		return OrbitalSimulationComponent;
	}

	/*----------------------------------------------------
	    Internals
	----------------------------------------------------*/

protected:
	/** Spacecraft data just replicated */
	UFUNCTION()
	void OnSpacecraftReplicated();

	/** Update the local database */
	void UpdateDatabase();

	/*----------------------------------------------------
	    Components
	----------------------------------------------------*/

protected:
	// Global orbital simulation manager
	UPROPERTY(Category = Nova, VisibleDefaultsOnly, BlueprintReadOnly)
	class UNovaOrbitalSimulationComponent* OrbitalSimulationComponent;

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

private:
	// Spacecraft array
	UPROPERTY(ReplicatedUsing = OnSpacecraftReplicated)
	TArray<FNovaSpacecraft> AISpacecraft;

	// Spacecraft array for player ships
	UPROPERTY(ReplicatedUsing = OnSpacecraftReplicated)
	TArray<FNovaSpacecraft> PlayerSpacecraft;

	// Local state
	TMap<FGuid, FNovaSpacecraftDatabaseEntry> SpacecraftDatabase;
	TArray<const class UNovaArea*>            Areas;
};