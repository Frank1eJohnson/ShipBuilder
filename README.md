# Spaceship Builder

Spaceship Builder is a shipbuilding game coming to Open Source since 2020.




![Game screenshot](https://shipbuilder.com/gallery_data/2.jpg)

## Building the game

Game sources are intended for game customers and Unreal Engine developers. **You won't be able to run the game from this repository alone**, as the game contents are not included. Building from source is only useful if you want to replace the game executable with your own modifications.

### Dependencies
You will need the following tools to build from sources:

* Spaceship Builder uses Unreal Engine 5 as a game engine. You can get it for free at [unrealengine.com](http://unrealengine.com). You need to download the appropriate Engine Engine 5 version from sources on GitHub - check the .uproject file as text for the current version in the "EngineAssociation" field.
* [Visual Studio Community 2022](https://www.visualstudio.com/downloads) will be used to build the game. You need the latest MSVC compiler and the Windows 10 SDK.

### Build process
Follow those steps to build the game:

* In the Windows explorer, right-click *ShipBuilder.uproject* and pick the "Generate Visual Studio Project Files" option.
* An *ShipBuilder.sln* file will appear - double-click it to open Visual Studio.
* Select the "Shipping" build type.
* You can now build the game by hitting F7 or using the Build menu. This should take a few minutes.

The resulting binary will be generated as *Binaries\Win64\ShipBuilder-Win64-Shipping.exe* and can replace the equivalent file in your existing game folder.
