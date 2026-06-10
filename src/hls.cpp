#include "sc/hls.hpp"
#include "sc/crypto.hpp"

#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <stdexcept>

namespace sc {

HlsDownloader::HlsDownloader(HttpClient& http) : http_(http) {}

HlsPlaylist HlsDownloader::parse_m3u8(const std::string& m3u8_content,
                                       const std::string& base_url) {
    HlsPlaylist playlist;
    std::istringstream stream(m3u8_content);
    std::string line;

    std::string current_key_uri;
    std::string current_iv;
    double current_duration = 0.0;

    auto resolve_url = [&](const std::string& uri) -> std::string {
        if (uri.find("http") == 0) return uri;
        if (uri.front() == '/') {
            auto scheme_end = base_url.find("://");
            if (scheme_end != std::string::npos) {
                auto host_end = base_url.find('/', scheme_end + 3);
                return base_url.substr(0, host_end) + uri;
            }
        }
        auto last_slash = base_url.rfind('/');
        if (last_slash != std::string::npos) {
            return base_url.substr(0, last_slash + 1) + uri;
        }
        return uri;
    };

    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();

        if (line.find("#EXT-X-KEY") == 0) {
            std::regex method_re(R"(METHOD=([^,]+))");
            std::regex uri_re(R"re(URI="([^"]+)")re");
            std::regex iv_re(R"re(IV=0x([0-9a-fA-F]+))re");

            std::smatch match;
            std::string method;
            if (std::regex_search(line, match, method_re)) {
                method = match[1].str();
            }

            if (method == "AES-128") {
                if (std::regex_search(line, match, uri_re)) {
                    current_key_uri = resolve_url(match[1].str());
                    if (!playlist.global_key_uri) {
                        playlist.global_key_uri = current_key_uri;
                    }
                }
                if (std::regex_search(line, match, iv_re)) {
                    current_iv = match[1].str();
                    if (!playlist.global_iv_hex) {
                        playlist.global_iv_hex = current_iv;
                    }
                }
            } else if (method == "NONE") {
                current_key_uri.clear();
                current_iv.clear();
            }
        } else if (line.find("#EXT-X-MAP") == 0) {
            std::regex map_uri_re(R"re(URI="([^"]+)")re");
            std::smatch match;
            if (std::regex_search(line, match, map_uri_re)) {
                playlist.init_segment_uri = resolve_url(match[1].str());
            }
        } else if (line.find("#EXTINF:") == 0) {
            auto comma = line.find(',');
            auto dur_str = line.substr(8, comma - 8);
            current_duration = std::stod(dur_str);
        } else if (!line.empty() && line[0] != '#') {
            HlsSegment seg;
            seg.uri = resolve_url(line);
            seg.duration = current_duration;
            if (!current_key_uri.empty()) {
                seg.key_uri = current_key_uri;
            }
            if (!current_iv.empty()) {
                seg.iv_hex = current_iv;
            }
            playlist.segments.push_back(std::move(seg));
            current_duration = 0.0;
        }
    }

    return playlist;
}

namespace {

std::vector<uint8_t> hex_to_bytes(const std::string& hex) {
    std::vector<uint8_t> bytes;
    for (size_t i = 0; i < hex.size(); i += 2) {
        auto byte_str = hex.substr(i, 2);
        bytes.push_back(static_cast<uint8_t>(std::stoul(byte_str, nullptr, 16)));
    }
    return bytes;
}

std::vector<uint8_t> sequence_to_iv(int sequence) {
    std::vector<uint8_t> iv(16, 0);
    iv[15] = static_cast<uint8_t>(sequence & 0xFF);
    iv[14] = static_cast<uint8_t>((sequence >> 8) & 0xFF);
    iv[13] = static_cast<uint8_t>((sequence >> 16) & 0xFF);
    iv[12] = static_cast<uint8_t>((sequence >> 24) & 0xFF);
    return iv;
}

} // namespace

bool HlsDownloader::download_stream(const std::string& m3u8_url,
                                     const std::filesystem::path& output_path,
                                     ProgressCallback progress) {
    auto m3u8_response = http_.get(m3u8_url);
    if (!m3u8_response.ok()) {
        throw std::runtime_error("Failed to fetch M3U8 playlist: HTTP " +
                                 std::to_string(m3u8_response.status_code));
    }

    auto playlist = parse_m3u8(m3u8_response.body, m3u8_url);

    if (playlist.segments.empty()) {
        throw std::runtime_error("No segments found in M3U8 playlist");
    }

    std::vector<uint8_t> encryption_key;
    std::string last_key_uri;

    std::ofstream out(output_path, std::ios::binary);
    if (!out) {
        throw std::runtime_error("Cannot open output file: " + output_path.string());
    }

    if (playlist.init_segment_uri) {
        auto init_data = http_.get_binary(*playlist.init_segment_uri);
        out.write(reinterpret_cast<const char*>(init_data.data()),
                  static_cast<std::streamsize>(init_data.size()));
    }

    size_t total = playlist.segments.size();
    for (size_t i = 0; i < total; ++i) {
        auto& seg = playlist.segments[i];

        if (seg.key_uri && *seg.key_uri != last_key_uri) {
            encryption_key = http_.get_binary(*seg.key_uri);
            last_key_uri = *seg.key_uri;
            if (encryption_key.size() != 16) {
                throw std::runtime_error("Invalid AES key size: " +
                                         std::to_string(encryption_key.size()));
            }
        }

        auto segment_data = http_.get_binary(seg.uri);

        if (!encryption_key.empty()) {
            std::vector<uint8_t> iv;
            if (seg.iv_hex) {
                iv = hex_to_bytes(*seg.iv_hex);
            } else {
                iv = sequence_to_iv(static_cast<int>(i));
            }

            segment_data = aes_128_cbc_decrypt(segment_data, encryption_key, iv);
        }

        out.write(reinterpret_cast<const char*>(segment_data.data()),
                  static_cast<std::streamsize>(segment_data.size()));

        if (progress) {
            progress(i + 1, total);
        }
    }

    out.close();
    return true;
}

} // namespace sc
