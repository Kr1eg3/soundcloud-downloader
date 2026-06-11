#include "sc/api.hpp"

#include <nlohmann/json.hpp>
#include <stdexcept>

namespace sc {

using json = nlohmann::json;

SoundCloudApi::SoundCloudApi(HttpClient& http, const std::string& client_id)
    : http_(http), client_id_(client_id) {}

namespace {

std::string json_str(const json& j, const std::string& key) {
    if (j.contains(key) && j[key].is_string()) {
        return j[key].get<std::string>();
    }
    return {};
}

TrackInfo parse_track(const json& j) {
    TrackInfo t;
    t.id = j.value("id", int64_t(0));
    t.title = json_str(j, "title");

    if (j.contains("user") && j["user"].is_object()) {
        t.artist = json_str(j["user"], "username");
    }

    t.permalink_url = json_str(j, "permalink_url");
    t.artwork_url = json_str(j, "artwork_url");
    t.duration_ms = j.value("duration", 0);
    t.is_streamable = j.value("streamable", false);

    return t;
}

} // namespace

std::variant<TrackInfo, PlaylistInfo> SoundCloudApi::resolve(const std::string& url) {
    auto encoded_url = url;
    std::string api_url = "https://api-v2.soundcloud.com/resolve?url=" +
                          encoded_url + "&client_id=" + client_id_;

    auto response = http_.get(api_url);
    if (!response.ok()) {
        throw std::runtime_error("Failed to resolve URL: HTTP " +
                                 std::to_string(response.status_code));
    }

    auto j = json::parse(response.body);
    auto kind = json_str(j, "kind");

    if (kind == "track") {
        return parse_track(j);
    }

    if (kind == "playlist") {
        PlaylistInfo pl;
        pl.id = j.value("id", int64_t(0));
        pl.title = json_str(j, "title");

        if (j.contains("user") && j["user"].is_object()) {
            pl.artist = json_str(j["user"], "username");
        }

        pl.permalink_url = json_str(j, "permalink_url");

        std::vector<int64_t> incomplete_ids;

        if (j.contains("tracks") && j["tracks"].is_array()) {
            for (auto& tj : j["tracks"]) {
                if (tj.contains("title") && !tj["title"].is_null()) {
                    pl.tracks.push_back(parse_track(tj));
                } else {
                    incomplete_ids.push_back(tj.value("id", int64_t(0)));
                }
            }
        }

        if (!incomplete_ids.empty()) {
            auto hydrated = hydrate_tracks(incomplete_ids);
            pl.tracks.insert(pl.tracks.end(), hydrated.begin(), hydrated.end());
        }

        return pl;
    }

    throw std::runtime_error("Unknown resource kind: " + kind);
}

std::string SoundCloudApi::get_stream_url(const TrackInfo& track) {
    std::string api_url = "https://api-v2.soundcloud.com/tracks/" +
                          std::to_string(track.id) + "?client_id=" + client_id_;

    auto response = http_.get(api_url);
    if (!response.ok()) {
        throw std::runtime_error("Failed to get track info: HTTP " +
                                 std::to_string(response.status_code));
    }

    auto j = json::parse(response.body);

    if (!j.contains("media") || !j["media"].contains("transcodings")) {
        throw std::runtime_error("No transcodings available for track: " + track.title);
    }

    std::string transcoding_url;
    for (auto& tc : j["media"]["transcodings"]) {
        auto protocol = tc.value("format", json::object()).value("protocol", std::string());
        if (protocol.find("hls") != std::string::npos) {
            transcoding_url = tc.value("url", std::string());
            break;
        }
    }

    if (transcoding_url.empty()) {
        throw std::runtime_error("No HLS transcoding found for track: " + track.title);
    }

    std::string stream_api_url = transcoding_url +
        (transcoding_url.find('?') != std::string::npos ? "&" : "?") +
        "client_id=" + client_id_;

    auto stream_response = http_.get(stream_api_url);
    if (!stream_response.ok()) {
        throw std::runtime_error("Failed to get stream URL: HTTP " +
                                 std::to_string(stream_response.status_code));
    }

    auto sj = json::parse(stream_response.body);
    auto m3u8_url = sj.value("url", std::string());

    if (m3u8_url.empty()) {
        throw std::runtime_error("Empty stream URL for track: " + track.title);
    }

    return m3u8_url;
}

std::vector<uint8_t> SoundCloudApi::download_artwork(const std::string& artwork_url) {
    if (artwork_url.empty()) return {};

    std::string hd_url = artwork_url;
    auto pos = hd_url.find("-large");
    if (pos != std::string::npos) {
        hd_url.replace(pos, 6, "-t500x500");
    }

    try {
        return http_.get_binary(hd_url);
    } catch (...) {
        try {
            return http_.get_binary(artwork_url);
        } catch (...) {
            return {};
        }
    }
}

std::vector<TrackInfo> SoundCloudApi::hydrate_tracks(const std::vector<int64_t>& ids) {
    std::vector<TrackInfo> result;

    for (size_t i = 0; i < ids.size(); i += 50) {
        std::string id_list;
        for (size_t j = i; j < ids.size() && j < i + 50; ++j) {
            if (!id_list.empty()) id_list += ",";
            id_list += std::to_string(ids[j]);
        }

        std::string api_url = "https://api-v2.soundcloud.com/tracks?ids=" +
                              id_list + "&client_id=" + client_id_;

        auto response = http_.get(api_url);
        if (!response.ok()) continue;

        auto j = json::parse(response.body);
        if (j.is_array()) {
            for (auto& tj : j) {
                result.push_back(parse_track(tj));
            }
        }
    }

    return result;
}

} // namespace sc
