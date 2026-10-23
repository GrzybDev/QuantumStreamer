# Quantum Streamer

Hook for Quantum Break that plays the live-action episodes from local copies and overrides their captions.

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

- The project contains no original assets from the game.
- To use this project you need to have an original copy of the game (bought from [Steam](https://store.steampowered.com/app/474960/Quantum_Break/)), the project doesn't make piracy easier and doesn't break any of the DRM included in-game.
- The project is not affiliated with or endorsed by Remedy Entertainment or Microsoft.

Features
--------

- Offline playback of the live-action episodes downloaded with [QuantumFetcher](https://github.com/GrzybDev/QuantumFetcher)
- Caption overrides: the episodes' captions can be translated or edited in SubRip files ([QuantumFetcher](https://github.com/GrzybDev/QuantumFetcher) exports the original captions)
- Optionally remove closed-caption annotations and music notes from overridden captions

Build Requirements
------------------

- Visual Studio 2026

### Libraries (installed by vcpkg):
- Poco
- GoogleTest (unit tests)

Compiling
---------

See the [shared build workflows](https://github.com/GrzybDev/actions/tree/v1/.github/workflows) `cpp-checks.yml` and `cpp-dll.yml`, which CI and the release build run, for more detailed steps

1. [Integrate vcpkg with Visual Studio](https://learn.microsoft.com/vcpkg/commands/integrate)
2. Build the solution `QuantumStreamer.slnx` in Visual Studio as x64 Release; the hook is written to `x64\Release\loc_x64_f.dll`
3. Run `x64\Release\QuantumStreamer.Tests.exe` to run the unit tests
4. To also check the game's own files, set the `QUANTUMSTREAMER_GAME_DIR` environment variable to the game folder before running the tests; these tests read its video lists and stored caption overrides and change nothing

Installing
----------

The hook takes the place of the game's `loc_x64_f.dll` and forwards every call to the original library, renamed to `loc_x64_f_o.dll`.

1. In the game's `dx11` folder, rename the original `loc_x64_f.dll` to `loc_x64_f_o.dll`
2. Copy `loc_x64_f.dll` there, either from the latest build in [GitHub Releases](https://github.com/GrzybDev/QuantumStreamer/releases) or from `x64\Release` if you compiled it yourself

To uninstall:

1. Delete the hook's `loc_x64_f.dll` from the `dx11` folder and rename `loc_x64_f_o.dll` back to `loc_x64_f.dll`
2. Copy the kept video list (`data/videoList_original.rmdj` by default) over `data/videoList.rmdj`, since the patched list points at the hook's local server; verifying the game files in Steam restores `loc_x64_f.dll` and `data/videoList.rmdj` as well

Configuration
-------------

You can change the default behavior of the hook by creating a config file named after the game executable, `QuantumBreak`, next to it (in the `dx11` folder) or in the game folder.

The following formats are supported:

- `QuantumBreak.properties` - properties file
- `QuantumBreak.ini` - initialization file
- `QuantumBreak.json` - JSON file
- `QuantumBreak.xml` - XML file

In the config, you can set the following values (a dot separates the section and the key). Relative paths start from the game folder. An invalid logging value keeps its default and is reported in the log; an invalid value of any other setting is reported in the log, and the hook does not start its server.


| Key                               | Description                                                                                   | Allowed values                                                                    | Default                          |
|:---------------------------------:|:---------------------------------------------------------------------------------------------:|:---------------------------------------------------------------------------------:|:---------------------------------|
| Logger.ShowConsole                | Show hook log in a console window; closing that window closes the game                        | Boolean                                                                           | false                            |
| Logger.SaveToLogFile              | Save hook log to file                                                                         | Boolean                                                                           | false                            |
| Logger.LogFile                    | Path of the log file                                                                          | String                                                                            | `QuantumStreamer.log`            |
| Logger.LogLevel_Core              | Changes how detailed Core logging is                                                          | [Poco::Message::Priority](https://docs.pocoproject.org/current/Poco.Message.html) | 6 (PRIO_INFORMATION)             |
| Logger.LogLevel_Network           | Changes how detailed Network logging is (HTTP Requests/Responses)                             | [Poco::Message::Priority](https://docs.pocoproject.org/current/Poco.Message.html) | 6 (PRIO_INFORMATION)             |
| Logger.LogLevel_VideoList         | Changes how detailed Video List subsystem logging is                                          | [Poco::Message::Priority](https://docs.pocoproject.org/current/Poco.Message.html) | 6 (PRIO_INFORMATION)             |
| Logger.LogLevel_OfflineStreaming  | Changes how detailed Offline Streaming subsystem logging is                                   | [Poco::Message::Priority](https://docs.pocoproject.org/current/Poco.Message.html) | 6 (PRIO_INFORMATION)             |
| Logger.LogLevel_SubtitleOverride  | Changes how detailed Subtitle Override subsystem logging is                                   | [Poco::Message::Priority](https://docs.pocoproject.org/current/Poco.Message.html) | 6 (PRIO_INFORMATION)             |
| Server.EpisodesPath               | Folder with one subfolder per stored episode                                                  | String                                                                            | `./videos/episodes`              |
| Server.MaxQueued                  | Max queued HTTP requests                                                                      | Integer                                                                           | 100                              |
| Server.MaxThreads                 | Max threads (HTTP server)                                                                     | Integer                                                                           | Logical CPU count or 2 if failed |
| Server.OfflineMode                | Disable online streaming, episodes stored locally will continue to work                       | Boolean                                                                           | false                            |
| Server.Port                       | Port the HTTP server listens on at `127.0.0.1` (game has to point to this port), 0 picks a free port | Unsigned short                                                               | 0                                |
| Server.VideoListPath              | Path to original, unmodified `./data/videoList.rmdj` file                                     | String                                                                            | `./data/videoList_original.rmdj` |
| Subtitles.ClosedCaptioning        | Keep closed-caption annotations such as `[ door slams ]` in overridden captions               | Boolean                                                                           | false                            |
| Subtitles.MusicNotes              | Keep `♪♪` music notes in overridden captions                                                  | Boolean                                                                           | true                             |
| VideoList.PatchFile               | Patch `./data/videoList.rmdj` to point to server on startup                                   | Boolean                                                                           | true                             |

Example config that disables online streaming and keeps closed-caption annotations:

```
[Server]
OfflineMode=true

[Subtitles]
ClosedCaptioning=true
```

### Episode folders

Each stored episode has its own folder, named after the episode, under `Server.EpisodesPath` (for example `videos/episodes/J1A-X1-X2` for episode `J1A-X1-X2`). [QuantumFetcher](https://github.com/GrzybDev/QuantumFetcher) writes these folders in the Smooth Streaming server layout the hook reads:

| File                                            | Content                                                                     |
|:------------------------------------------------|:----------------------------------------------------------------------------|
| `<episode>.ism`                                 | the server manifest; its `head` names the client manifest                   |
| `<episode>.ismc`                                | the client manifest the game receives                                       |
| `<episode>_<stream>_<bitrate>.ismv`, `.isma`, `.ismt` | the video, audio and caption tracks the server manifest lists, each ending with an `mfra` fragment index |

The server manifest names the client manifest like this:

```xml
<head>
  <meta name="clientManifestRelativePath" content="J1A-X1-X2.ismc"/>
</head>
```

A track that is missing or has no valid fragment index is skipped; its fragments are then fetched online unless `Server.OfflineMode` is on.

### Caption overrides

A SubRip file in an episode folder whose name contains `_captions` replaces the caption track of the same name, for example `enus_captions.srt` replaces the `enus_captions` track. The file is UTF-8 and holds numbered cues, each with its times and one or more lines of text, separated by blank lines:

```
1
00:00:07,500 --> 00:00:09,833
EPISODE 1

2
00:00:10,960 --> 00:00:12,500
[ door slams ]
- Who is there?
```

Text is shown as written: there are no escapes, and markup such as `<i>` appears literally. A cue whose end time cannot be read is never shown. Annotations in brackets with spaces, such as `[ door slams ]`, and `♪♪` music notes are kept or removed as `Subtitles.ClosedCaptioning` and `Subtitles.MusicNotes` say.

Usage
-----

1. Install the hook and, if needed, create the config file
2. Put the episodes downloaded with [QuantumFetcher](https://github.com/GrzybDev/QuantumFetcher) and any caption overrides into `Server.EpisodesPath`
3. Start the game; with the default config, episodes that are not stored keep streaming online as before

When the game starts, the hook:

1. Reads the config file and opens a local HTTP server on `127.0.0.1`
2. Keeps the game's own `data/videoList.rmdj` as `Server.VideoListPath` if no copy is kept yet, then rewrites `data/videoList.rmdj` to point at the local server
3. Indexes the stored episodes and caption overrides
4. Serves every episode request: stored files from disk, everything else from the online service unless `Server.OfflineMode` is on, with overridden caption tracks rewritten on the way

With `VideoList.PatchFile` set to false, `data/videoList.rmdj` is left alone and has to point at the hook already, for example after `quantumfetcher videolist patch` from [QuantumFetcher](https://github.com/GrzybDev/QuantumFetcher). Set `Server.Port` to the port that list uses (QuantumFetcher's default is 10000), and keep the store's copy at `Server.VideoListPath`, since the hook reads the episodes' online addresses from it.

A kept copy that already points at the local server no longer holds the episodes' online addresses, so the hook removes it and leaves `data/videoList.rmdj` as it is; verify the game files in Steam to restore the original list. When the kept copy cannot be read, `data/videoList.rmdj` is left alone as well.

Credits
-------

- [GrzybDev](https://grzyb.dev)

Thanks to:
- [r00t0](https://github.com/cleverzaq) (for help with decoding `videoList.rmdj`)

Special thanks to:
- Remedy Entertainment (for making the game)
- Microsoft Studios (for publishing the game on PC)
