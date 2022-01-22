// Spaceship Builder - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Game/NovaGameTypes.h"

#include "NovaGameInstance.generated.h"

/** Game instance class */
UCLASS(ClassGroup = (Nova))
class UNovaGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UNovaGameInstance();

	/*----------------------------------------------------
	    Loading & saving
	----------------------------------------------------*/

	TSharedPtr<struct FNovaGameSave> Save(const class ANovaPlayerController* PC);

	void Load(TSharedPtr<struct FNovaGameSave> SaveData);

	static void SerializeJson(
		TSharedPtr<struct FNovaGameSave>& SaveData, TSharedPtr<class FJsonObject>& JsonData, ENovaSerialize Direction);

	/*----------------------------------------------------
	    Inherited & callbacks
	----------------------------------------------------*/

	virtual void Init() override;

	virtual void Shutdown() override;

	void PreLoadMap(const FString& InMapName);

	/*----------------------------------------------------
	    Game save handling
	----------------------------------------------------*/

	/** Start the game from a save file */
	void StartGame(FString SaveName, bool Online = true);

	/** Try loading the game from the save slot if it exists, or create a new one */
	void LoadGame(FString SaveName);

	/** Save the game to the currently loaded slot */
	void SaveGame(class ANovaPlayerController* PC, bool Synchronous = false);

	/** Save the current save data to the currently loaded slot */
	void SaveGameToFile(bool Synchronous = false);

	/** Check that the current save data is valid */
	bool HasSave() const;

	/** Get the time in minutes since the last loading or saving */
	double GetMinutesSinceLastSave() const
	{
		double CurrentTime = FPlatformTime::ToMilliseconds64(FPlatformTime::Cycles64());

		return (CurrentTime - TimeOfLastSave) / (1000.0 * 60.0);
	}

	/** Get the save data for the player */
	TSharedPtr<struct FNovaPlayerSave> GetPlayerSave();

	/** Get the save data for the world */
	TSharedPtr<struct FNovaGameStateSave> GetWorldSave();

	/** Get the save data for the contract system*/
	TSharedPtr<struct FNovaContractManagerSave> GetContractManagerSave();

	/*----------------------------------------------------
	    Game flow
	----------------------------------------------------*/

	/** Get the catalog */
	class UNovaAssetManager* GetAssetManager() const
	{
		return AssetManager;
	}

	/** Get the contract manager */
	class UNovaContractManager* GetContractManager() const
	{
		return ContractManager;
	}

	/** Get the sessions manager */
	class UNovaSessionsManager* GetSessionsManager() const
	{
		return SessionsManager;
	}

	/** Get the menu manager */
	class UNovaMenuManager* GetMenuManager() const
	{
		return MenuManager;
	}

	/** Get the sound manager */
	class UNovaSoundManager* GetSoundManager() const
	{
		return SoundManager;
	}

	/** Restart the game from the level in save data */
	void SetGameOnline(FString URL, bool Online = true);

	/** Exit the session and go to the main menu */
	void GoToMainMenu();

	/** Change level on the server */
	void ServerTravel(FString URL);

private:
	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

	// Asset manager object
	UPROPERTY()
	class UNovaAssetManager* AssetManager;

	// Save manager object
	UPROPERTY()
	class UNovaSaveManager* SaveManager;

	// Sessions manager object
	UPROPERTY()
	class UNovaSessionsManager* SessionsManager;

	// Menu manager object
	UPROPERTY()
	class UNovaMenuManager* MenuManager;

	// Sound manager object
	UPROPERTY()
	class UNovaSoundManager* SoundManager;

	// Contract manager object
	UPROPERTY()
	class UNovaContractManager* ContractManager;

	// Save data
	TSharedPtr<struct FNovaGameSave> CurrentSaveData;
	FString                          CurrentSaveFileName;
	double                           TimeOfLastSave;
};
