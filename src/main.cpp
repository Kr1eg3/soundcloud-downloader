#include "sc/api.hpp"
#include "sc/client_id.hpp"
#include "sc/downloader.hpp"
#include "sc/hls.hpp"
#include "sc/http.hpp"
#include "sc/transcoder.hpp"
#include "sc/types.hpp"
#include "sc/utils.hpp"

#include <CLI/CLI.hpp>
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    CLI::App app{"sc-dl: SoundCloud Track & Playlist Downloader"};
    app.set_version_flag("--version", "0.1.0");

    std::string url;
    std::string format_str = "mp3";
    std::string output_dir = ".";
    std::string manual_client_id;
    bool overwrite = false;
    bool quiet = false;

    app.add_option("url", url, "SoundCloud track or playlist URL")->required();
    app.add_option("-f,--format", format_str, "Output format: mp3 or m4a (default: mp3)");
    app.add_option("-o,--output", output_dir, "Output directory (default: current)");
    app.add_option("--client-id", manual_client_id, "Manual client_id override");
    app.add_flag("--overwrite", overwrite, "Overwrite existing files");
    app.add_flag("-q,--quiet", quiet, "Suppress progress output");

    CLI11_PARSE(app, argc, argv);

    if (!sc::is_soundcloud_url(url)) {
        std::cerr << "Error: Invalid SoundCloud URL.\n";
        return 1;
    }

    sc::OutputFormat format = sc::OutputFormat::MP3;
    if (format_str == "m4a") {
        format = sc::OutputFormat::M4A;
    } else if (format_str != "mp3") {
        std::cerr << "Error: Unknown format '" << format_str << "'. Use mp3 or m4a.\n";
        return 1;
    }

    if (format == sc::OutputFormat::MP3 && !sc::Transcoder::is_available()) {
        std::cerr << "Error: FFmpeg is required for MP3 output but was not found in PATH.\n"
                  << "Install it: winget install ffmpeg\n"
                  << "Or use --format m4a to skip transcoding.\n";
        return 1;
    }

    sc::DownloadOptions opts;
    opts.url = url;
    opts.format = format;
    opts.output_dir = output_dir;
    opts.overwrite = overwrite;
    opts.quiet = quiet;

    try {
        sc::HttpClient http;

        std::string client_id;
        if (!manual_client_id.empty()) {
            client_id = manual_client_id;
        } else {
            if (!quiet) std::cout << "Resolving client_id...\n";
            client_id = sc::extract_client_id(http);
        }

        sc::SoundCloudApi api(http, client_id);
        sc::HlsDownloader hls(http);
        sc::Transcoder transcoder;
        sc::Downloader downloader(api, hls, transcoder);

        if (!quiet) std::cout << "Resolving URL...\n";
        auto result = api.resolve(url);

        if (auto* track = std::get_if<sc::TrackInfo>(&result)) {
            if (!quiet) {
                std::cout << "Track: \"" << track->title << "\" by " << track->artist
                          << " (" << (track->duration_ms / 60000) << ":"
                          << ((track->duration_ms / 1000) % 60) << ")\n";
            }
            if (!downloader.download_track(*track, opts)) {
                return 1;
            }
        } else if (auto* playlist = std::get_if<sc::PlaylistInfo>(&result)) {
            if (!quiet) {
                std::cout << "Playlist: \"" << playlist->title << "\" by " << playlist->artist
                          << " (" << playlist->tracks.size() << " tracks)\n";
            }
            downloader.download_playlist(*playlist, opts);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
