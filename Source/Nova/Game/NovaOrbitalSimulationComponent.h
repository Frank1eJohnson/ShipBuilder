// Nova project - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "NovaArea.h"
#include "NovaOrbitalSimulationTypes.h"
#include "Nova/Game/NovaGameTypes.h"

#include "NovaOrbitalSimulationComponent.generated.h"

/** Orbital simulation component that ticks orbiting spacecraft */
UCLASS(ClassGroup = (Nova))
class UNovaOrbitalSimulationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNovaOrbitalSimulationComponent();

	/*----------------------------------------------------
	    General simulation
	----------------------------------------------------*/
public:
	virtual void BeginPlay() override;

	/** Update the simulation */
	void UpdateSimulation();

	/** Get the current time */
	FNovaTime GetCurrentTime() const;

	/** Get the time at which the next player maneuver will occur */
	FNovaTime GetTimeOfNextPlayerManeuver() const
	{
		return TimeOfNextPlayerManeuver;
	}

	/*----------------------------------------------------
	    Trajectory & orbiting interface
	----------------------------------------------------*/

public:
	/** Build trajectory parameters from an arbitrary orbit to another  */
	TSharedPtr<struct FNovaTrajectoryParameters> PrepareTrajectory(const TSharedPtr<FNovaOrbit>& Source,
		const TSharedPtr<FNovaOrbit>& Destination, FNovaTime DeltaTime, const TArray<FGuid>& SpacecraftIdentifiers) const;

	/** Compute a trajectory */
	TSharedPtr<FNovaTrajectory> ComputeTrajectory(const TSharedPtr<struct FNovaTrajectoryParameters>& Parameters, float PhasingAltitude);

	/** Check if this spacecraft is on a trajectory */
	bool IsOnStartedTrajectory(const FGuid& SpacecraftIdentifier) const;

	/** Check if this trajectory can be started */
	bool CanCommitTrajectory(const TSharedPtr<FNovaTrajectory>& Trajectory, FText* Help = nullptr) const;

	/** Put spacecraft on a new trajectory */
	void CommitTrajectory(const TArray<FGuid>& SpacecraftIdentifiers, const TSharedPtr<FNovaTrajectory>& Trajectory);

	/** Complete the current trajectory of spacecraft */
	void CompleteTrajectory(const TArray<FGuid>& SpacecraftIdentifiers);

	/** Abort the current trajectory of spacecraft */
	void AbortTrajectory(const TArray<FGuid>& SpacecraftIdentifiers);

	/** Put spacecraft in a particular orbit */
	void SetOrbit(const TArray<FGuid>& SpacecraftIdentifiers, const TSharedPtr<FNovaOrbit>& Orbit);

	/** Merge different spacecraft in a particular orbit */
	void MergeOrbit(const TArray<FGuid>& SpacecraftIdentifiers, const TSharedPtr<FNovaOrbit>& Orbit);

	/*----------------------------------------------------
	    Simple trajectory & orbiting getters
	----------------------------------------------------*/

	/** Get this component */
	static UNovaOrbitalSimulationComponent* Get(const UObject* Outer);

	/** Get the orbital data for an area */
	TSharedPtr<FNovaOrbit> GetAreaOrbit(const UNovaArea* Area) const
	{
		return MakeShared<FNovaOrbit>(FNovaOrbitGeometry(Area->Body, Area->Altitude, Area->Phase), FNovaTime());
	}

	/** Get an area's location */
	const FNovaOrbitalLocation& GetAreaLocation(const UNovaArea* Area) const
	{
		return AreaOrbitalLocations[Area];
	}

	/** Get all area's locations */
	const TMap<const UNovaArea*, FNovaOrbitalLocation>& GetAllAreasLocations() const
	{
		return AreaOrbitalLocations;
	}

	/** Get a spacecraft's orbit */
	const FNovaOrbit* GetSpacecraftOrbit(const FGuid& Identifier) const
	{
		return SpacecraftOrbitDatabase.Get(Identifier);
	}

	/** Get a spacecraft's trajectory */
	const FNovaTrajectory* GetSpacecraftTrajectory(const FGuid& Identifier) const
	{
		return SpacecraftTrajectoryDatabase.Get(Identifier);
	}

	/** Get a spacecraft's index in a trajectory */
	int32 GetSpacecraftTrajectoryIndex(const FGuid& Identifier) const
	{
		return SpacecraftTrajectoryDatabase.GetSpacecraftIndex(Identifier);
	}

	/** Get a player spacecraft's index in a trajectory */
	int32 GetPlayerSpacecraftIndex(const FGuid& Identifier) const;

	/** Get a spacecraft's location */
	const FNovaOrbitalLocation* GetSpacecraftLocation(const FGuid& Identifier) const
	{
		return SpacecraftOrbitalLocations.Find(Identifier);
	}

	/** Get all spacecraft's locations */
	const TMap<FGuid, FNovaOrbitalLocation>& GetAllSpacecraftLocations() const
	{
		return SpacecraftOrbitalLocations;
	}

	/*----------------------------------------------------
	    Advanced trajectory & orbiting getters
	----------------------------------------------------*/

	/** Get the player orbit */
	const FNovaOrbit* GetPlayerOrbit() const;

	/** Get the player trajectory */
	const FNovaTrajectory* GetPlayerTrajectory() const;

	/** Get the player location */
	const FNovaOrbitalLocation* GetPlayerLocation() const;

	/** Get the time left until a maneuver happens */
	FNovaTime GetTimeLeftUntilPlayerManeuver() const
	{
		return GetTimeOfNextPlayerManeuver() - GetCurrentTime() - FNovaTime::FromMinutes(TimeMarginBeforeManeuver);
	}

	/** Is the player past the first maneuver of a trajectory */
	bool IsPlayerPastFirstManeuver() const
	{
		const FNovaTrajectory* Trajectory = GetPlayerTrajectory();

		return Trajectory && Trajectory->GetFirstManeuverStartTime() < GetCurrentTime();
	}

	/** Is the player nearing the last maneuver of a trajectory */
	bool IsPlayerNearingLastManeuver() const
	{
		const FNovaTrajectory* Trajectory = GetPlayerTrajectory();

		return Trajectory && Trajectory->GetRemainingManeuverCount(GetCurrentTime()) == 1;
	}

	/** Get the closest area and the associated distance from an arbitrary location */
	TPair<const UNovaArea*, float> GetNearestAreaAndDistance(const FNovaOrbitalLocation& Location) const;

	/** Get the closest area and the associated distance from the player*/
	TPair<const UNovaArea*, float> GetPlayerNearestAreaAndDistance() const
	{
		const FNovaTrajectory*      PlayerTrajectory = GetPlayerTrajectory();
		const FNovaOrbitalLocation* PlayerLocation   = GetPlayerLocation();

		if (PlayerTrajectory)
		{
			if (PlayerLocation)
			{
				return GetNearestAreaAndDistance(*PlayerLocation);
			}
		}

		return TPair<const UNovaArea*, float>(nullptr, MAX_FLT);
	}

	/** Get the current thrust factor for a spacecraft */
	float GetCurrentSpacecraftThrustFactor(const FGuid& Identifier) const
	{
		const FNovaTrajectory* Trajectory = GetSpacecraftTrajectory(Identifier);

		if (Trajectory)
		{
			const FNovaManeuver* Maneuver = Trajectory->GetCurrentManeuver(GetCurrentTime());
			if (Maneuver)
			{
				int32 SpacecraftIndex = GetSpacecraftTrajectoryIndex(Identifier);
				NCHECK(SpacecraftIndex != INDEX_NONE && SpacecraftIndex >= 0 && SpacecraftIndex < Maneuver->ThrustFactors.Num());
				return Maneuver->ThrustFactors[SpacecraftIndex];
			}
		}

		return 0;
	}

	/*----------------------------------------------------
	    Internals
	----------------------------------------------------*/

protected:
	/** Clean up obsolete orbit data */
	void ProcessOrbitCleanup();

	/** Update all area's position */
	void ProcessAreas();

	/** Update the current orbit of spacecraft */
	void ProcessSpacecraftOrbits();

	/** Update the current trajectory of spacecraft */
	void ProcessSpacecraftTrajectories();

	/** Compute the period of a stable circular orbit */
	static FNovaTime GetOrbitalPeriod(const double GravitationalParameter, const double SemiMajorAxis)
	{
		return FNovaTime::FromMinutes(2.0 * PI * sqrt(pow(SemiMajorAxis, 3.0) / GravitationalParameter) / 60.0);
	}

	/*----------------------------------------------------
	    Properties
	----------------------------------------------------*/

public:
	// Delay after a trajectory has started before removing the orbit data
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	float OrbitGarbageCollectionDelay;

	// Time in minutes to leave players before a maneuver
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	double TimeMarginBeforeManeuver;

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

private:
	// Replicated orbit database
	UPROPERTY(Replicated) FNovaOrbitDatabase SpacecraftOrbitDatabase;

	// Replicated trajectory database
	UPROPERTY(Replicated)
	FNovaTrajectoryDatabase SpacecraftTrajectoryDatabase;

	// Local object state
	TArray<const class UNovaArea*>                     Areas;
	TMap<const class UNovaArea*, FNovaOrbitalLocation> AreaOrbitalLocations;
	TMap<FGuid, FNovaOrbitalLocation>                  SpacecraftOrbitalLocations;

	// Local state
	FNovaTime TimeOfNextPlayerManeuver;
};
