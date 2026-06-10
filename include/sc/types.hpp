#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace sc {

struct TrackInfo {
    int64_t id = 0;
    std::string title;
    std::string artist;
    std::string permalink_url;
    std::string artwork_url;
    int duration_ms = 0;
    bool is_streamable = false;
};

struct PlaylistInfo {
    int64_t id = 0;
    std::string title;
    std::string artist;
    std::string permalink_url;
    std::vector<TrackInfo> tracks;
};

enum class OutputFormat { MP3, M4A };

struct DownloadOptions {
    std::string url;
    OutputFormat format = OutputFormat::MP3;
    std::filesystem::path output_dir = std::filesystem::current_path();
    bool overwrite = false;
    bool quiet = false;
    std::optional<std::string> client_id_override;
};

struct HlsSegment {
    std::string uri;
    double duration = 0.0;
    std::optional<std::string> key_uri;
    std::optional<std::string> iv_hex;
};

struct HlsPlaylist {
    std::vector<HlsSegment> segments;
    std::optional<std::string> global_key_uri;
    std::optional<std::string> global_iv_hex;
    std::optional<std::string> init_segment_uri;
};

} // namespace sc
