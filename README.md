# Quantum Streamer

Hook for Quantum Break that enables offline playback of live episodes and subtitle override.

Table of Contents
-----------------
- [Game Info](#game-info)
- [Legal notes](#legal-notes)
- [Features](#features)
- [Build Requirements](#build-requirements)
- [Compiling](#compiling)
- [Installing](#installing)
- [Configuration](#configuration)
- [Usage](#usage)
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

Features
--------

- Offline playback (see [QuantumFetcher](https://github.com/GrzybDev/QuantumFetcher.git))
- Subtitles override support (you can translate/edit subtitles in episodes, please use [QuantumFetcher](https://github.com/GrzybDev/QuantumFetcher.git) to get the original subtitles)
- Dynamically add/remove closed captions
- Dynamically add/remove music notes in captions
- Dynamically add/remove episode title caption to title card

Build Requirements
------------------

- Visual Studio 2022

### Libraries:
- Poco

Compiling
---------

See one of [build workflows](https://github.com/GrzybDev/QuantumStreamer/blob/main/.github/workflows) for more detailed steps

1. [Integrate vcpkg with Visual Studio](https://learn.microsoft.com/vcpkg/commands/integrate)
2. Build project in Visual Studio (It has to be x64 Release build, it's the only one configuration setup, but please check before doing that)

Installing
----------

- Rename original `loc_x64_f.dll` to `loc_x64_f_o.dll`
- Copy `loc_x64_f.dll` file from either the latest build in [GitHub Releases](https://github.com/GrzybDev/QuantumStreamer/releases) or from `Release` folder if you compiled it yourself

Configuration
-------------

You can change the default behavior of the hook by creating config in supported format where the game .exe is

The following formats are supported:

- .properties - properties file (in order to use it, create `QuantumBreak.properties` where the game executable is)
- .ini - initialization file (in order to use it, create `QuantumBreak.ini` where the game executable is)
- .xml - XML file (in order to use it, create `QuantumBreak.xml` where the game executable is)

In the config, you can set following values (dot seperates section and key)


| Key                               | Description                                                                                   | Allowed Values                                                                    | Default Value                    |
|:---------------------------------:|:---------------------------------------------------------------------------------------------:|:---------------------------------------------------------------------------------:|:---------------------------------|
| Logger.ShowConsole                | Show hook log in console                                                                      | Boolean                                                                           | false                            |
| Logger.SaveToLogFile              | Save hook log to file                                                                         | Boolean                                                                           | false                            |
| Logger.LogFile                    | Path to where save log file                                                                   | String                                                                            | `QuantumStreamer.log`            |
| Logger.LogLevel_Core              | Changes how detailed Core logging is                                                          | [Poco::Message::Priority](https://docs.pocoproject.org/current/Poco.Message.html) | 6 (PRIO_INFORMATION)             |
| Logger.LogLevel_Network           | Changes how detailed Network logging is (HTTP Requests/Responses)                             | [Poco::Message::Priority](https://docs.pocoproject.org/current/Poco.Message.html) | 6 (PRIO_INFORMATION)             |
| Logger.LogLevel_VideoList         | Changes how detailed Video List subsystem logging is                                          | [Poco::Message::Priority](https://docs.pocoproject.org/current/Poco.Message.html) | 6 (PRIO_INFORMATION)             |
| Logger.LogLevel_OfflineStreaming  | Changes how detailed Offline Streaming subsystem logging is                                   | [Poco::Message::Priority](https://docs.pocoproject.org/current/Poco.Message.html) | 6 (PRIO_INFORMATION)             |
| Logger.LogLevel_SubtitleOverride  | Changes how detailed Subtitle Override subsystem logging is                                   | [Poco::Message::Priority](https://docs.pocoproject.org/current/Poco.Message.html) | 6 (PRIO_INFORMATION)             |
| Server.EpisodesPath               | Path to where episodes data are located                                                       | String                                                                            | `./videos/episodes`              |
| Server.MaxQueued                  | Max queued HTTP requests                                                                      | Integer                                                                           | 100                              |
| Server.MaxThreads                 | Max threads (HTTP server)                                                                     | Integer                                                                           | Logical CPU count or 2 if failed |
| Server.OfflineMode                | Disable online streaming, episodes stored locally will continue to work                       | Boolean                                                                           | false                            |
| Server.Port                       | Port for HTTP server (game also have to point to this port), if 0 will use random unused port | Unsigned short                                                                    | 0                                |
| Server.VideoListPath              | Path to original, unmodified `./data/videoList.rmdj` file                                     | String                                                                            | `./data/videoList_original.rmdj` |
| Subtitles.ClosedCaptioning        | Show closed captions in subtitles                                                             | Boolean                                                                           | false                            |
| Subtitles.MusicNotes              | Show music notes in subtitles                                                                 | Boolean                                                                           | true                             |
| Subtitles.EpisodeTitles           | Append episode title to text streams                                                          | Boolean                                                                           | true                             |
| VideoList.PatchFile               | Patch `./data/videoList.rmdj` to point to server on startup                                   | Boolean                                                                           | true                             |

The default config should work for most of the users, but if you have special requirements you can change above settings.

Example config that will disable online streaming and enables Closed Captioning:

```
[Server]
OfflineMode=false

[Subtitles]
ClosedCaptioning=true
```

Usage
-----

Initially, after installing the hook, with default config - nothing should change in-game.
Quantum Streamer on game launch will try to load `Server.VideoListPath`, after that hook will scan `Server.EpisodesPath` directory for the locally stored episodes and/or captions_overrides.

VideoList loaded by game (`data/videoList.rmdj`) has to point to the Quantum Streamer server, by default Quantum Streamer will patch this file, but you can disable this behaviour by setting `VideoList.PatchFile` to false, but then, you have to patch that file manually - for that, you can use [QuantumFetcher](https://github.com/GrzybDev/QuantumFetcher.git) tool.

Each episode data is expected to be in dedicated episode directory (e.g. `./videos/episodes/J1A-X1-X2` is where hook will look for episode `J1A-X1-X2` (assuming `Server.EpisodesPath` is not changed))
Local episodes are expected to be in ISM Smooth Stream format:

`*.ism` (Server Manifest) should at least define `clientManifestRelativePath` in `head` section which should reference filename/relative path for client manifest file.
All media files referenced in the Server Manifest will be loaded (if media file exist)

Additionally, JSON or BSON files which are named `*_captions_override.?son` will be loaded, after that hook will replace captions in specific track (e.g. `enus_captions_override.json` will override `enus_captions` track) with the ones from the file, allowing you to translate or edit captions in the live action.

Both JSON and BSON files are expected to have the same structure, example:

```
{
  "episode_title": "Episode title goes here",
  "segments": ["This is segment 0 (s0)", "This is segment 1 (s1)"]
}
```

Credits
-------

- [GrzybDev](https://grzyb.dev)

Special thanks to:
- Remedy Entertainment (for making the game)
- Microsoft Studios (for publishing the game on PC)
- [r00t0](https://github.com/cleverzaq) - For help with decoding `videoList.rmdj`
