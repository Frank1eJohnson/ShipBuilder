// Nova project - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerStart.h"

#include "NovaPlayerStart.generated.h"

/** Player start actor */
UCLASS(ClassGroup = (Nova))
class ANovaPlayerStart : public APlayerStart
{
	GENERATED_BODY()

public:
	ANovaPlayerStart(const FObjectInitializer& ObjectInitializer);

	/*----------------------------------------------------
	    Public methods
	----------------------------------------------------*/
public:
#if WITH_EDITOR

	virtual void Tick(float DeltaTime) override;

	virtual bool ShouldTickIfViewportsOnly() const override
	{
		return true;
	}

#endif

	/** Get the world location of the dock waiting point */
	FVector GetWaitingPointLocation() const
	{
		return WaitingPoint->GetComponentLocation();
	}

	/** Get the world location of the area enter point */
	FVector GetEnterPointLocation(float DeltaV, float Acceleration) const
	{
		const float StoppingTime     = FMath::Abs(DeltaV) / Acceleration;
		const float StoppingDistance = 100 * StoppingTime * (DeltaV / 2);
		return GetWaitingPointLocation() + GetEnterPointDirection(DeltaV) * StoppingDistance;
	}

	/** Get the world direction of the area enter point */
	FVector GetEnterPointDirection(float DeltaV) const
	{
		return (DeltaV > 0 ? -1 : 1) * FVector(0, 1, 0);
	}

	/** Get the world location of the area exit point */
	FVector GetExitPointLocation(float DeltaV, float Acceleration) const
	{
		const float StoppingTime     = FMath::Abs(DeltaV) / Acceleration;
		const float StoppingDistance = 100 * StoppingTime * (DeltaV / 2);
		return GetWaitingPointLocation() + GetExitPointDirection(DeltaV) * StoppingDistance;
	}

	/** Get the world direction of the area exit point */
	FVector GetExitPointDirection(float DeltaV) const
	{
		return (DeltaV > 0 ? -1 : 1) * FVector(0, -1, 0);
	}

	/*----------------------------------------------------
	    Components
	----------------------------------------------------*/

protected:
	// Waiting point for spacecraft that just left dock or is waiting for docking
	UPROPERTY(Category = Nova, VisibleDefaultsOnly, BlueprintReadOnly)
	USceneComponent* WaitingPoint;

	/*----------------------------------------------------
	    Properties
	----------------------------------------------------*/

public:
	// Whether ships here should start idle in space
	UPROPERTY(Category = Nova, EditAnywhere)
	bool IsInSpace;
};
