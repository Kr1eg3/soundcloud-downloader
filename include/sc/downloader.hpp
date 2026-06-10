#pragma once

#include "sc/api.hpp"
#include "sc/hls.hpp"
#include "sc/tagger.hpp"
#include "sc/transcoder.hpp"
#include "sc/types.hpp"

namespace sc {

class Downloader {
public:
    Downloader(SoundCloudApi& api, HlsDownloader& hls,
               Transcoder& transcoder);

    bool download_track(const TrackInfo& track, const DownloadOptions& opts);
    void download_playlist(const PlaylistInfo& playlist, const DownloadOptions& opts);

private:
    SoundCloudApi& api_;
    HlsDownloader& hls_;
    Transcoder& transcoder_;
};

} // namespace sc
