// Spaceship Builder - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "Game/NovaGameTypes.h"
#include "Spacecraft/NovaSpacecraft.h"
#include "NovaAISpacecraft.generated.h"

/** Description of an AI spacecraft class */
UCLASS(ClassGroup = (Nova))
class UNovaAISpacecraftDescription : public UNovaAssetDescription
{
	GENERATED_BODY()

public:
	/** Load this asset to the running game */
	UFUNCTION(Category = Nova, BlueprintCallable, CallInEditor)
	void LoadInGame();

	/** Save this asset from the running game */
	UFUNCTION(Category = Nova, BlueprintCallable, CallInEditor)
	void SaveFromGame();

private:
	/** Fetch the player's spacecraft pawn */
	class ANovaSpacecraftPawn* GetSpacecraftPawn() const;

public:
	UPROPERTY()
	FNovaSpacecraft Spacecraft;
};
