// Spaceship Builder - Gwennaël Arbona

#pragma once

#include "EngineMinimal.h"
#include "NovaGameTypes.h"
#include "NovaAsteroid.generated.h"

/** Asteroid catalog & metadata */
UCLASS(ClassGroup = (Nova))
class UNovaAsteroidConfiguration : public UNovaAssetDescription
{
	GENERATED_BODY()

public:
	// Body orbited
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	const class UNovaCelestialBody* Body;

	// Total amount of asteroids to spawn in the entire game
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	int32 TotalAsteroidCount;

	// Orbital altitude distribution
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	const class UCurveFloat* AltitudeDistribution;

	// Meshes to use on asteroids
	UPROPERTY(Category = Assets, EditDefaultsOnly)
	TArray<TSoftObjectPtr<class UStaticMesh>> Meshes;
};

/** Asteroid data object */
struct FNovaAsteroid
{
	FNovaAsteroid() : Identifier(FGuid::NewGuid()), Body(nullptr), Altitude(0), Phase(0), Mesh()
	{}

	FNovaAsteroid(FRandomStream& RandomStream, const class UNovaCelestialBody* B, double A, double P)
		: Identifier(FGuid(RandomStream.RandHelper(MAX_int32), RandomStream.RandHelper(MAX_int32), RandomStream.RandHelper(MAX_int32),
			  RandomStream.RandHelper(MAX_int32)))
		, Body(B)
		, Altitude(A)
		, Phase(P)
		, Mesh()
	{}

	// Identifier
	FGuid Identifier;

	// Orbital parameters
	const class UNovaCelestialBody* Body;
	double                          Altitude;
	double                          Phase;

	// Physical characteristics
	TSoftObjectPtr<class UStaticMesh> Mesh;
};

/** Asteroid spawning helper */
struct FNovaAsteroidSpawner
{
	/** Reset the spawner */
	void Start(const UNovaAsteroidConfiguration* Configuration);

	/** Check if a new asteroid can be created and get its details */
	bool GetNextAsteroid(double& Altitude, double& Phase, FNovaAsteroid& Asteroid);

private:
	const UNovaAsteroidConfiguration* AsteroidConfiguration;
	int32                             SpawnedAsteroids;
	FRandomStream                     RandomStream;
	TMultiMap<int32, double>          IntegralToAltitudeTable;
};
