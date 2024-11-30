# Quantum Streamer

Hook for Quantum Break that enables offline playback of live episodes and subtitle override.

Table of Contents
-----------------
- [Game Info](#game-info)
- [Legal notes](#legal-notes)
- [Build Requirements](#build-requirements)
- [Compiling](#compiling)
- [Installing](#installing)
- [Credits](#credits)

Game Info
---------
![Quantum Break Cover](https://upload.wikimedia.org/wikipedia/en/d/d9/Quantum_Break_cover.jpg "Quantum Break Cover")

|         Type | Value                                                        |
|-------------:|:-------------------------------------------------------------|
| Developer(s) | Remedy Entertainment                                         |
| Publisher(s) | Microsoft Studios                                            |
|  Director(s) | Sam Lake, Mikael Kasurinen                                   |
|  Producer(s) | Miloš Jeřábek                                                |
|       Engine | Northlight Engine                                            |
|  Platform(s) | Windows, Xbox One                                            |
|     Genre(s) | Action-adventure, third-person shooter                       |
|      Mode(s) | Single-player                                                |

Legal notes
-----------

- The project doesn't contain ***any*** original assets from the game!
- To use this project you need to have an original copy of the game (bought from [Steam](https://store.steampowered.com/app/474960/Quantum_Break/)), the project doesn't make piracy easier and doesn't break any of the DRM included in-game.

Build Requirements
------------------

- Visual Studio 2022

### Libraries:
- Boost
- pugixml

Compiling
---------

See [build workflow](https://github.com/GrzybDev/QuantumStreamer/blob/main/.github/workflows/build.yaml) for more detailed steps

1. [Integrate vcpkg with Visual Studio](https://learn.microsoft.com/vcpkg/commands/integrate)
2. Build project in Visual Studio (It has to be x64 Release build, it's the only one configuration setup, but please check before doing that)

Installing
----------

Copy all `dll` files from either the latest build in [GitHub Releases](https://github.com/GrzybDev/QuantumStreamer/releases) or from `Release` folder if you compiled it yourself to the game root folder (where `exe` file is located)

Credits
-------

- [GrzybDev](https://grzyb.dev)

Special thanks to:
- Remedy Entertainment (for making the game)
- Microsoft Studios (for publishing the game on PC)
- [r00t0](https://github.com/cleverzaq) - For help with decoding `videoList.rmdj`
