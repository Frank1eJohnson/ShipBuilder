// Nova project - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "NovaCaptureActor.generated.h"

/** Camera control pawn for the factory view */
UCLASS(Config = Game, ClassGroup = (Nova))
class ANovaCaptureActor : public AActor
{
	GENERATED_BODY()

public:
	ANovaCaptureActor();

	/*----------------------------------------------------
	    Gameplay
	----------------------------------------------------*/

	/** Render this asset and save it */
	void RenderAsset(class UNovaAssetDescription* Asset, struct FSlateBrush& AssetRender);

protected:
#if WITH_EDITOR

	/** Spawn the assembly */
	void CreateSpacecraft();

	/** Spawn the mesh viewer */
	void CreateMeshActor();

	/** Get a catalog instance if not already existing */
	void CreateAssetManager();

	/** Spawn a new render target */
	void CreateRenderTarget();

	/** Set the camera to a better location */
	void PlaceCamera();

	/** Save the render target to a texture */
	class UTexture2D* SaveTexture(FString TextureName);

	/** Get the texture desired size */
	FVector2D GetDesiredSize() const;

#endif    // WITH_EDITOR

	/*----------------------------------------------------
	    Properties
	----------------------------------------------------*/

#if WITH_EDITORONLY_DATA

public:
	// Empty compartment kit
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	class UNovaCompartmentDescription* EmptyCompartmentDescription;

	// Upscale factor to apply to rendering
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	int32 RenderUpscaleFactor;

	// Upscale factor to apply to the image itself
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	int32 ResultUpscaleFactor;

	/*----------------------------------------------------
	    Components
	----------------------------------------------------*/

protected:
	// Camera arm
	UPROPERTY(Category = Nova, VisibleDefaultsOnly, BlueprintReadOnly)
	class USceneComponent* CameraArmComponent;

	// Camera capture component
	UPROPERTY(Category = Nova, VisibleDefaultsOnly, BlueprintReadOnly)
	class USceneCaptureComponent2D* CameraCapture;

protected:
	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

	// Spacecraft pawn
	UPROPERTY()
	class ANovaSpacecraftPawn* SpacecraftPawn;

	// Simple mesh
	UPROPERTY()
	class AStaticMeshActor* MeshActor;

	// Simple mesh
	UPROPERTY()
	class AActor* TargetActor;

	// Asset manager
	UPROPERTY(Transient)
	class UNovaAssetManager* AssetManager;

	// Render target used for rendering the assets
	UPROPERTY(Transient)
	class UTextureRenderTarget2D* RenderTarget;

	// General data
	TSharedPtr<struct FNovaSpacecraft> Spacecraft;

#endif    // WITH_EDITORONLY_DATA
};
