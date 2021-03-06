// Nova project - Gwennaël Arbona

#include "NovaSpacecraftPropellantSystem.h"

#include "Nova/Game/NovaOrbitalSimulationComponent.h"

#include "Net/UnrealNetwork.h"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

UNovaSpacecraftPropellantSystem::UNovaSpacecraftPropellantSystem()
	: Super()

	, PropellantRate(0)
	, PropellantMass(0)
{
	SetIsReplicatedByDefault(true);
}

/*----------------------------------------------------
    System implementation
----------------------------------------------------*/

void UNovaSpacecraftPropellantSystem::Update(FNovaTime InitialTime, FNovaTime FinalTime)
{
	NCHECK(GetOwner()->GetLocalRole() == ROLE_Authority);

	const UNovaOrbitalSimulationComponent*  Simulation        = UNovaOrbitalSimulationComponent::Get(this);
	const FNovaSpacecraftPropulsionMetrics* PropulsionMetrics = GetPropulsionMetrics();
	const FGuid&                            Identifier        = GetSpacecraftIdentifier();

	// Process usage between start and end time
	if (Simulation && PropulsionMetrics)
	{
		const FNovaTrajectory* Trajectory         = Simulation->GetSpacecraftTrajectory(Identifier);
		const float            FullPropellantRate = PropulsionMetrics->PropellantRate;
		FNovaTime              CurrentTime        = InitialTime;

		PropellantRate = 0;

		if (Trajectory)
		{
			for (const FNovaManeuver& Maneuver : Trajectory->Maneuvers)
			{
				FNovaTime ManeuverEndTime = FMath::Min(Maneuver.Time + Maneuver.Duration, FinalTime);

				if (CurrentTime >= Maneuver.Time && CurrentTime <= ManeuverEndTime)
				{
					int32 SpacecraftIndex = Simulation->GetSpacecraftTrajectoryIndex(Identifier);
					NCHECK(SpacecraftIndex != INDEX_NONE && SpacecraftIndex >= 0 && SpacecraftIndex < Maneuver.ThrustFactors.Num());
					PropellantRate = FullPropellantRate * Maneuver.ThrustFactors[SpacecraftIndex];

					double DeltaTimeSeconds = (ManeuverEndTime - CurrentTime).AsMinutes() * 60;

					PropellantMass -= PropellantRate * DeltaTimeSeconds;

#if 0
					NLOG("Rate %f, dt %f, current consumed %f", CurrentRate, DeltaTimeSeconds, ConsumedAmount);
#endif

					CurrentTime = ManeuverEndTime;
				}
			}
		}

		NCHECK(PropellantMass >= 0);
	}
}

void UNovaSpacecraftPropellantSystem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UNovaSpacecraftPropellantSystem, PropellantRate);
	DOREPLIFETIME(UNovaSpacecraftPropellantSystem, PropellantMass);
}
