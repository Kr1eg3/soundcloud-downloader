#include "sc/downloader.hpp"
#include "sc/progress.hpp"
#include "sc/utils.hpp"

#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace sc {

Downloader::Downloader(SoundCloudApi& api, HlsDownloader& hls,
                       Transcoder& transcoder)
    : api_(api), hls_(hls), transcoder_(transcoder) {}

bool Downloader::download_track(const TrackInfo& track, const DownloadOptions& opts) {
    std::string ext = (opts.format == OutputFormat::MP3) ? ".mp3" : ".m4a";
    std::string filename = sanitize_filename(track.artist + " - " + track.title) + ext;
    auto final_path = opts.output_dir / filename;

    if (std::filesystem::exists(final_path) && !opts.overwrite) {
        if (!opts.quiet) {
            std::cout << "Skipping (already exists): " << filename << "\n";
        }
        return true;
    }

    auto stream_url = api_.get_stream_url(track);
    auto temp_aac = get_temp_path(".aac");

    if (!opts.quiet) {
        std::cout << "Downloading...\n";
    }

    ProgressCallback progress_cb = nullptr;
    if (!opts.quiet) {
        progress_cb = [&track](size_t current, size_t total) {
            int percent = static_cast<int>((current * 100) / total);
            int bar_width = 30;
            int filled = (bar_width * percent) / 100;
            std::cout << "\r  [";
            for (int i = 0; i < bar_width; ++i) {
                std::cout << (i < filled ? '#' : '-');
            }
            std::cout << "] " << percent << "% (" << current << "/" << total << " segments)";
            std::cout.flush();
        };
    }

    hls_.download_stream(stream_url, temp_aac, progress_cb);
    if (!opts.quiet) std::cout << "\n";

    std::filesystem::create_directories(opts.output_dir);

    bool transcode_ok = false;
    if (opts.format == OutputFormat::MP3) {
        if (!opts.quiet) std::cout << "Converting to MP3...\n";
        transcode_ok = transcoder_.to_mp3(temp_aac, final_path);
    } else {
        if (!opts.quiet) std::cout << "Muxing to M4A...\n";
        transcode_ok = transcoder_.to_m4a(temp_aac, final_path);
    }

    std::filesystem::remove(temp_aac);

    if (!transcode_ok) {
        std::cerr << "Error: Transcoding failed for \"" << track.title << "\"\n";
        return false;
    }

    if (!opts.quiet) std::cout << "Tagging metadata...\n";
    auto artwork = api_.download_artwork(track.artwork_url);
    tag_file(final_path, track, artwork);

    if (!opts.quiet) {
        std::cout << "Saved: " << filename << "\n";
    }

    return true;
}

void Downloader::download_playlist(const PlaylistInfo& playlist,
                                    const DownloadOptions& opts) {
    auto dir_name = sanitize_filename(playlist.artist + " - " + playlist.title);
    auto playlist_dir = opts.output_dir / dir_name;
    std::filesystem::create_directories(playlist_dir);

    int total = static_cast<int>(playlist.tracks.size());
    int success = 0;
    int failed = 0;

    for (int i = 0; i < total; ++i) {
        auto& track = playlist.tracks[i];

        if (!opts.quiet) {
            std::cout << "\n[" << (i + 1) << "/" << total << "] \""
                      << track.title << "\" by " << track.artist << "\n";
        }

        std::string ext = (opts.format == OutputFormat::MP3) ? ".mp3" : ".m4a";
        std::ostringstream prefix;
        prefix << std::setw(2) << std::setfill('0') << (i + 1);
        std::string filename = prefix.str() + " - " +
                              sanitize_filename(track.title) + ext;

        DownloadOptions track_opts = opts;
        track_opts.output_dir = playlist_dir;

        try {
            auto stream_url = api_.get_stream_url(track);
            auto temp_aac = get_temp_path(".aac");

            ProgressCallback progress_cb = nullptr;
            if (!opts.quiet) {
                progress_cb = [](size_t current, size_t tot) {
                    int percent = static_cast<int>((current * 100) / tot);
                    int bar_width = 30;
                    int filled = (bar_width * percent) / 100;
                    std::cout << "\r  [";
                    for (int j = 0; j < bar_width; ++j) {
                        std::cout << (j < filled ? '#' : '-');
                    }
                    std::cout << "] " << percent << "%";
                    std::cout.flush();
                };
            }

            hls_.download_stream(stream_url, temp_aac, progress_cb);
            if (!opts.quiet) std::cout << "\n";

            auto final_path = playlist_dir / filename;
            bool ok = false;
            if (opts.format == OutputFormat::MP3) {
                ok = transcoder_.to_mp3(temp_aac, final_path);
            } else {
                ok = transcoder_.to_m4a(temp_aac, final_path);
            }
            std::filesystem::remove(temp_aac);

            if (ok) {
                auto artwork = api_.download_artwork(track.artwork_url);
                tag_file(final_path, track, artwork);
                ++success;
                if (!opts.quiet) std::cout << "  Done.\n";
            } else {
                ++failed;
                std::cerr << "  Error: Transcoding failed.\n";
            }
        } catch (const std::exception& e) {
            ++failed;
            std::cerr << "  Error: " << e.what() << " Skipping.\n";
        }
    }

    if (!opts.quiet) {
        std::cout << "\nComplete: " << success << "/" << total
                  << " tracks downloaded to " << playlist_dir.string() << "/\n";
        if (failed > 0) {
            std::cout << failed << " track(s) skipped due to errors.\n";
        }
    }
}

} // namespace sc
