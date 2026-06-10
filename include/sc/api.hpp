#pragma once

#include "sc/http.hpp"
#include "sc/types.hpp"

#include <string>
#include <variant>
#include <vector>

namespace sc {

class SoundCloudApi {
public:
    SoundCloudApi(HttpClient& http, const std::string& client_id);

    std::variant<TrackInfo, PlaylistInfo> resolve(const std::string& url);
    std::string get_stream_url(const TrackInfo& track);
    std::vector<uint8_t> download_artwork(const std::string& artwork_url);

private:
    HttpClient& http_;
    std::string client_id_;

    std::vector<TrackInfo> hydrate_tracks(const std::vector<int64_t>& ids);
};

} // namespace sc
