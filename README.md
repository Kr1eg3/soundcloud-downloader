# sc-dl

A command-line tool for downloading tracks and playlists from SoundCloud.

## Download

**Windows (x64):** grab the latest `sc-dl-*.zip` from [Releases](../../releases/latest), unzip, and put `sc-dl.exe` somewhere in your PATH. The only external requirement is [FFmpeg](https://www.gyan.dev/ffmpeg/builds/):

```
winget install ffmpeg
```

## Features

- Download single tracks by URL
- Download entire playlists by URL
- Output format: MP3 or M4A (native AAC, no re-encoding)
- Automatic ID3/MP4 metadata tagging (title, artist, cover art)
- Progress bar for each download
- Automatic `client_id` extraction and caching

## Usage

```
sc-dl <url> [OPTIONS]
```

### Options

| Flag | Description |
|---|---|
| `-f, --format <fmt>` | Output format: `mp3` or `m4a` (default: `mp3`) |
| `-o, --output <dir>` | Output directory (default: current) |
| `--overwrite` | Overwrite existing files |
| `--client-id <id>` | Manual client_id override |
| `-q, --quiet` | Suppress progress output |
| `-h, --help` | Show help |
| `--version` | Show version |

### Examples

```bash
# Download a track as MP3
sc-dl https://soundcloud.com/artist/track-name

# Download as M4A (faster, no transcoding)
sc-dl https://soundcloud.com/artist/track-name -f m4a

# Download a playlist to a specific directory
sc-dl https://soundcloud.com/artist/sets/my-playlist -o D:\Music

# Download playlist as M4A, overwrite existing files
sc-dl https://soundcloud.com/artist/sets/my-playlist -f m4a -o ./downloads --overwrite
```

### Output

Single track:
```
Resolving client_id...
Resolving URL...
Track: "Song Title" by Artist Name (3:42)
Downloading...
  [######################--------] 73% (22/30 segments)
Converting to MP3...
Tagging metadata...
Saved: Artist Name - Song Title.mp3
```

Playlist:
```
Resolving URL...
Playlist: "My Playlist" by Artist Name (12 tracks)

[1/12] "Track One" by Artist...
  [##############################] 100%
  Done.

...

Complete: 12/12 tracks downloaded to ./Artist Name - My Playlist/
```

## Prerequisites

- **FFmpeg** — required for MP3 conversion and M4A muxing. Install via:
  ```
  winget install ffmpeg
  ```
  If FFmpeg is not found and format is MP3, the tool will exit with an error. M4A also requires FFmpeg for proper container muxing.

## Building

### Requirements

- Visual Studio 2022 or 2025 (MSVC, C++ workload)
- CMake 3.20+ (included with Visual Studio)
- Ninja (included with Visual Studio)
- [vcpkg](https://github.com/microsoft/vcpkg)
- Git (for FetchContent during CMake configure)

### Setup vcpkg (one-time)

```powershell
git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
C:\vcpkg\bootstrap-vcpkg.bat
```

Set the `VCPKG_ROOT` environment variable:

```powershell
[System.Environment]::SetEnvironmentVariable("VCPKG_ROOT", "C:\vcpkg", "User")
```

### Build

From a **Developer Command Prompt / PowerShell for Visual Studio**:

```powershell
cmake --preset msvc-release
cmake --build build/release
```

The executable is at `build/release/sc-dl.exe`.

For a debug build:

```powershell
cmake --preset msvc-debug
cmake --build build/debug
```

You can also open the project directly in Visual Studio via **File > Open > CMake** and select `CMakeLists.txt`.

### Dependencies

| Library | Source | Purpose |
|---|---|---|
| libcurl | vcpkg | HTTP/HTTPS client |
| OpenSSL | vcpkg | AES-128-CBC decryption of HLS segments |
| nlohmann-json | vcpkg | JSON parsing |
| TagLib | vcpkg | Audio metadata tagging (ID3v2, MP4) |
| CLI11 | FetchContent | Command-line argument parsing |

## How it works

1. Extracts a `client_id` from SoundCloud's JavaScript bundles (cached for 24 hours)
2. Resolves the input URL via the SoundCloud API v2 (`/resolve`)
3. For playlists, hydrates incomplete track metadata in batches of 50
4. Fetches HLS stream URL from track transcodings
5. Downloads the M3U8 playlist, then fetches and decrypts fMP4 segments (AES-128-CBC)
6. Muxes into M4A or transcodes to MP3 via FFmpeg
7. Tags the output file with title, artist, and cover art via TagLib

## License

MIT
