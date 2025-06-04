# Quantum Streamer

Hook for Quantum Break that enables offline playback of live episodes and subtitle override.

Table of Contents
-----------------
- [Game Info](#game-info)
- [Legal notes](#legal-notes)
- [Build Requirements](#build-requirements)
- [Compiling](#compiling)
- [Installing](#installing)
- [Configuration](#configuration)
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
- Poco

Compiling
---------

See [build workflow](https://github.com/GrzybDev/QuantumStreamer/blob/main/.github/workflows/build.yaml) for more detailed steps

1. [Integrate vcpkg with Visual Studio](https://learn.microsoft.com/vcpkg/commands/integrate)
2. Build project in Visual Studio (It has to be x64 Release build, it's the only one configuration setup, but please check before doing that)

Installing
----------

Copy all `dll` files from either the latest build in [GitHub Releases](https://github.com/GrzybDev/QuantumStreamer/releases) or from `Release` folder if you compiled it yourself to the game root folder (where `exe` file is located)

Configuration
-------------

You can change the default behavior of the hook by creating config in supported format where the game .exe is

The following formats are supported:

- .properties - properties file (in order to use it, create `QuantumBreak.properties` where the game executable is)
- .ini - initialization file (in order to use it, create `QuantumBreak.ini` where the game executable is)
- .xml - XML file (in order to use it, create `QuantumBreak.xml` where the game executable is)

In the config, you can set following values (dot seperates section and key)


| Key							| Description																								| Allowed Values														| Default Value						|
|-------------------------------|-----------------------------------------------------------------------------------------------------------|-----------------------------------------------------------------------|-----------------------------------|
| Hook.OriginalLibraryName		| Original DLL filename or path																				| Any string															| `loc_x64_f_o.dll`					|
| Logger.ShowConsole			| Show hook log in console																					| Boolean (true/false)													| `false`							|
| Logger.SaveToLogFile			| Save hook log to file																						| Boolean (true/false)													| `false`							|
| Logger.LogFile				| Filename of log file/Path to where save log file															| Any string															| `QuantumStreamer.log`				|
| Logger.LogLevel_Hook			| Changes how detailed hook logging is (hook here means all of the stuff that game loads this library for)	| Integer (Range 0-8) The higher the number the more verbose output is	| `1`								|
| Logger.LogLevel_Server		| Changes how detailed server logging is (all of the "actions" performed by server)							| Integer (Range 0-8) The higher the number the more verbose output is	| `1`								|
| Logger.LogLevel_HTTP			| Changes how detailed HTTP logging is (HTTP Requests/Responses)											| Integer (Range 0-8) The higher the number the more verbose output is	| `1`								|
| Server.EpisodesPath			| Path to where episodes data are located																	| Any string															| `./videos/episodes`				|
| Server.MaxQueued				| Max queued HTTP requests																					| Integer																| `100`								|
| Server.MaxThreads				| Max threads (HTTP server)																					| Integer																| `16`								|
| Server.OfflineMode			| Disable online streaming, episodes stored locally will continue to work.									| Boolean (true/false)													| `false`							|
| Server.Port					| Port for HTTP server (game also have to point to this port)												| Unsigned short (Range 0-65535)										| `10000`							|
| Server.VideoListPath			| Path to original `videoList.rmdj`																			| Any string															| `./data/videoList_original.rmdj`	|
| Subtitles.ClosedCaptioning	| If `false`, remove closed captions from subtitles															| Boolean (true/false)													| `false`							|
| Subtitles.MusicNotes			| If `false`, remove music notes from subtitles																| Boolean (true/false)													| `true`							|
| Subtitles.EpisodeNames		| If `true`, append episode name to video stream															| Boolean (true/false)													| `true`							|


Credits
-------

- [GrzybDev](https://grzyb.dev)

Special thanks to:
- Remedy Entertainment (for making the game)
- Microsoft Studios (for publishing the game on PC)
- [r00t0](https://github.com/cleverzaq) - For help with decoding `videoList.rmdj`
