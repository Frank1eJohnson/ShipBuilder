// Spaceship Builder - Gwennaël Arbona

#pragma once

#include "NovaGameMode.h"
#include "NovaGameState.h"

#include "NovaArea.h"
#include "NovaOrbitalSimulationComponent.h"

#include "Player/NovaPlayerController.h"
#include "Spacecraft/NovaSpacecraftPawn.h"
#include "Spacecraft/NovaSpacecraftMovementComponent.h"
#include "System/NovaAssetManager.h"

#include "Nova.h"

/*----------------------------------------------------
    Constants
----------------------------------------------------*/

// Cutscene timing values in seconds
namespace ENovaGameModeStateTiming
{
constexpr float CommonCutsceneDelay       = 2.0;
constexpr float DepartureCutsceneDuration = 5.0;
constexpr float AreaIntroductionDuration  = 5.0;
constexpr float ArrivalCutsceneDuration   = 5.0;
}    // namespace ENovaGameModeStateTiming

/*----------------------------------------------------
    Game state class
----------------------------------------------------*/

/** Game state */
class FNovaGameModeState
{
public:
	FNovaGameModeState() : PC(nullptr), GameState(nullptr), OrbitalSimulationComponent(nullptr)
	{}

	virtual ~FNovaGameModeState()
	{}

	/** Set the required work data */
	void Initialize(const FString& Name, class ANovaPlayerController* P, class ANovaGameMode* GM, class ANovaGameState* GW,
		class UNovaOrbitalSimulationComponent* OSC)
	{
		StateName                  = Name;
		PC                         = P;
		GameMode                   = GM;
		GameState                  = GW;
		OrbitalSimulationComponent = OSC;
	}

	/** Get the time spent in this state in minutes */
	double GetMinutesInState() const
	{
		return (GameState->GetCurrentTime() - StateStartTime).AsMinutes();
	}

	/** Get the time spent in this state in seconds */
	double GetSecondsInState() const
	{
		return (GameState->GetCurrentTime() - StateStartTime).AsSeconds();
	}

	/** Enter this state from a previous one */
	virtual void EnterState(ENovaGameStateIdentifier PreviousStateIdentifier)
	{
		NLOG("FNovaGameState::EnterState : entering state '%s'", *StateName);
		StateStartTime = GameState->GetCurrentTime();
	}

	/** Run the state and return the desired current state */
	virtual ENovaGameStateIdentifier UpdateState()
	{
		return static_cast<ENovaGameStateIdentifier>(-1);
	}

	/** Prepare to leave this state for a new one */
	virtual void LeaveState(ENovaGameStateIdentifier NewStateIdentifier)
	{}

protected:
	// Local state
	FString   StateName;
	FNovaTime StateStartTime;

	// Outer objects
	class ANovaPlayerController*           PC;
	class ANovaGameMode*                   GameMode;
	class ANovaGameState*                  GameState;
	class UNovaOrbitalSimulationComponent* OrbitalSimulationComponent;
};

/*----------------------------------------------------
    Idle states
----------------------------------------------------*/

// Area state : operating locally within an area
class FNovaAreaState : public FNovaGameModeState
{
public:
	virtual void EnterState(ENovaGameStateIdentifier PreviousState) override
	{
		FNovaGameModeState::EnterState(PreviousState);

		PC->SharedTransition(ENovaPlayerCameraState::Default,    //
			FNovaAsyncAction::CreateLambda(
				[&]()
				{
					GameState->SetUsingTrajectoryMovement(false);
				}));
	}

	virtual ENovaGameStateIdentifier UpdateState() override
	{
		FNovaGameModeState::UpdateState();

		const FNovaTrajectory* PlayerTrajectory = OrbitalSimulationComponent->GetPlayerTrajectory();

		if (PlayerTrajectory && PlayerTrajectory->GetFirstManeuverStartTime() <=
									GameState->GetCurrentTime() + FNovaTime::FromSeconds(ENovaGameModeStateTiming::CommonCutsceneDelay))
		{
			return ENovaGameStateIdentifier::DepartureProximity;
		}
		else
		{
			return ENovaGameStateIdentifier::Area;
		}
	}
};

// Orbit state : operating locally in empty space
class FNovaOrbitState : public FNovaGameModeState
{
public:
	virtual void EnterState(ENovaGameStateIdentifier PreviousState) override
	{
		FNovaGameModeState::EnterState(PreviousState);

		PC->SharedTransition(ENovaPlayerCameraState::Default,    //
			FNovaAsyncAction::CreateLambda(
				[&]()
				{
					GameMode->ChangeAreaToOrbit();
				}));
	}

	virtual ENovaGameStateIdentifier UpdateState() override
	{
		FNovaGameModeState::UpdateState();

		if (OrbitalSimulationComponent->GetTimeLeftUntilPlayerManeuver() <= FNovaTime() &&
			OrbitalSimulationComponent->IsPlayerNearingLastManeuver())
		{
			return ENovaGameStateIdentifier::ArrivalIntro;
		}
		else
		{
			return ENovaGameStateIdentifier::Orbit;
		}
	}
};

// Fast forward state : screen is blacked out, exit state depends on orbital simulation
class FNovaFastForwardState : public FNovaGameModeState
{
public:
	virtual void EnterState(ENovaGameStateIdentifier PreviousState) override
	{
		FNovaGameModeState::EnterState(PreviousState);

		PC->SharedTransition(ENovaPlayerCameraState::FastForward,    //
			FNovaAsyncAction::CreateLambda(
				[&]()
				{
					GameState->FastForward();
				}));
	}

	virtual ENovaGameStateIdentifier UpdateState() override
	{
		FNovaGameModeState::UpdateState();

		if (GameState->IsInFastForward())
		{
			return ENovaGameStateIdentifier::FastForward;
		}
		else if (OrbitalSimulationComponent->IsPlayerPastFirstManeuver())
		{
			GameMode->ResetSpacecraft();

			if (OrbitalSimulationComponent->IsPlayerNearingLastManeuver())
			{
				return ENovaGameStateIdentifier::ArrivalIntro;
			}
			else
			{
				return ENovaGameStateIdentifier::Orbit;
			}
		}
		else
		{
			GameMode->ResetSpacecraft();

			return ENovaGameStateIdentifier::Area;
		}
	}
};

/*----------------------------------------------------
    Area departure states
----------------------------------------------------*/

// Departure stage 1 : cutscene focused on spacecraft
class FNovaDepartureProximityState : public FNovaGameModeState
{
public:
	virtual void EnterState(ENovaGameStateIdentifier PreviousState) override
	{
		FNovaGameModeState::EnterState(PreviousState);

		PC->SharedTransition(ENovaPlayerCameraState::CinematicSpacecraft,    //
			FNovaAsyncAction::CreateLambda(
				[&]()
				{
					GameState->SetUsingTrajectoryMovement(true);
				}));
	}

	virtual ENovaGameStateIdentifier UpdateState() override
	{
		FNovaGameModeState::UpdateState();

		if (GetSecondsInState() > ENovaGameModeStateTiming::CommonCutsceneDelay + ENovaGameModeStateTiming::DepartureCutsceneDuration)
		{
			return ENovaGameStateIdentifier::DepartureCoast;
		}
		else
		{
			return ENovaGameStateIdentifier::DepartureProximity;
		}
	}
};

// Departure stage 2 : pit state, wait until we fast-forward or arrive naturally
class FNovaDepartureCoastState : public FNovaGameModeState
{
public:
	virtual void EnterState(ENovaGameStateIdentifier PreviousState) override
	{
		FNovaGameModeState::EnterState(PreviousState);

		PC->SharedTransition(ENovaPlayerCameraState::Default,    //
			FNovaAsyncAction::CreateLambda(
				[&]()
				{
					GameState->SetUsingTrajectoryMovement(false);
					GameMode->ChangeAreaToOrbit();
				}));
	}

	virtual ENovaGameStateIdentifier UpdateState() override
	{
		FNovaGameModeState::UpdateState();

		if (OrbitalSimulationComponent->GetTimeLeftUntilPlayerManeuver() <= FNovaTime() &&
			OrbitalSimulationComponent->IsPlayerNearingLastManeuver())
		{
			return ENovaGameStateIdentifier::ArrivalIntro;
		}
		else
		{
			return ENovaGameStateIdentifier::DepartureCoast;
		}
	}
};

/*----------------------------------------------------
    Area arrival states
----------------------------------------------------*/

// Arrival stage 1 : level loading and introduction
class FNovaArrivalIntroState : public FNovaGameModeState
{
public:
	virtual void EnterState(ENovaGameStateIdentifier PreviousState) override
	{
		FNovaGameModeState::EnterState(PreviousState);

		TPair<const UNovaArea*, float> NearestAreaAndDistance = OrbitalSimulationComponent->GetPlayerNearestAreaAndDistance();

		// Below 10km distance, we assume this 100% has to be a station
		// TODO : this won't work with spacecraft and so a travel target needs to be known
		IsStillInSpace = NearestAreaAndDistance.Value > 10;

		// If we're nearing a station, show the cinematic cutscene, else skip straight to coast state
		if (!IsStillInSpace)
		{
			PC->SharedTransition(ENovaPlayerCameraState::CinematicEnvironment,    //
				FNovaAsyncAction::CreateLambda(
					[&, NearestAreaAndDistance]()
					{
						GameMode->ChangeArea(NearestAreaAndDistance.Key);
					}));
		}
	}

	virtual ENovaGameStateIdentifier UpdateState() override
	{
		FNovaGameModeState::UpdateState();

		if (IsStillInSpace || GetSecondsInState() > ENovaGameModeStateTiming::AreaIntroductionDuration)
		{
			return ENovaGameStateIdentifier::ArrivalCoast;
		}
		else
		{
			return ENovaGameStateIdentifier::ArrivalIntro;
		}
	}

private:
	bool IsStillInSpace;
};

// Arrival stage 2 : coast until proximity
class FNovaArrivalCoastState : public FNovaGameModeState
{
public:
	virtual void EnterState(ENovaGameStateIdentifier PreviousState) override
	{
		FNovaGameModeState::EnterState(PreviousState);

		PC->SharedTransition(ENovaPlayerCameraState::Default,    //
			FNovaAsyncAction::CreateLambda(
				[&]()
				{
					GameMode->SetCurrentAreaVisible(false);
				}));
	}

	virtual ENovaGameStateIdentifier UpdateState() override
	{
		FNovaGameModeState::UpdateState();

		const FNovaTrajectory* PlayerTrajectory = OrbitalSimulationComponent->GetPlayerTrajectory();
		if (PlayerTrajectory == nullptr || (PlayerTrajectory->GetArrivalTime() - GameState->GetCurrentTime() <
											   FNovaTime::FromSeconds(ENovaGameModeStateTiming::ArrivalCutsceneDuration)))
		{
			return ENovaGameStateIdentifier::ArrivalProximity;
		}
		else
		{
			return ENovaGameStateIdentifier::ArrivalCoast;
		}
	}
};

// Arrival stage 3 : proximity
class FNovaArrivalProximityState : public FNovaGameModeState
{
public:
	virtual void EnterState(ENovaGameStateIdentifier PreviousState) override
	{
		FNovaGameModeState::EnterState(PreviousState);

		IsWaitingDelay = false;

		PC->SharedTransition(ENovaPlayerCameraState::CinematicSpacecraft,    //
			FNovaAsyncAction::CreateLambda(
				[&]()
				{
					GameMode->SetCurrentAreaVisible(true);
					GameState->SetUsingTrajectoryMovement(true);
				}));
	}

	virtual ENovaGameStateIdentifier UpdateState() override
	{
		FNovaGameModeState::UpdateState();

		if (IsWaitingDelay)
		{
			if (GameState->GetCurrentTime() - ArrivalTime > FNovaTime::FromSeconds(ENovaGameModeStateTiming::CommonCutsceneDelay))
			{
				return ENovaGameStateIdentifier::Area;
			}
		}
		else if (OrbitalSimulationComponent->GetPlayerTrajectory() == nullptr)
		{
			ArrivalTime    = GameState->GetCurrentTime();
			IsWaitingDelay = true;
		}

		return ENovaGameStateIdentifier::ArrivalProximity;
	}

private:
	bool      IsWaitingDelay;
	FNovaTime ArrivalTime;
};
