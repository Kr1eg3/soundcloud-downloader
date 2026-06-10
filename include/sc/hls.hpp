#pragma once

#include "sc/http.hpp"
#include "sc/types.hpp"

#include <filesystem>
#include <functional>
#include <string>

namespace sc {

class HlsDownloader {
public:
    explicit HlsDownloader(HttpClient& http);

    HlsPlaylist parse_m3u8(const std::string& m3u8_content,
                           const std::string& base_url);

    bool download_stream(const std::string& m3u8_url,
                         const std::filesystem::path& output_path,
                         ProgressCallback progress = nullptr);

private:
    HttpClient& http_;
};

} // namespace sc
