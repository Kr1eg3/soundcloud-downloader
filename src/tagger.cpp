#include "sc/tagger.hpp"

#include <taglib/fileref.h>
#include <taglib/mpegfile.h>
#include <taglib/id3v2tag.h>
#include <taglib/id3v2frame.h>
#include <taglib/attachedpictureframe.h>
#include <taglib/mp4file.h>
#include <taglib/mp4tag.h>
#include <taglib/mp4coverart.h>
#include <taglib/tstring.h>

namespace sc {

bool tag_file(const std::filesystem::path& audio_file,
              const TrackInfo& track,
              const std::vector<uint8_t>& artwork_data) {

    auto ext = audio_file.extension().string();

    if (ext == ".mp3") {
        TagLib::MPEG::File file(audio_file.string().c_str());
        if (!file.isValid()) return false;

        auto* tag = file.ID3v2Tag(true);
        tag->setTitle(TagLib::String(track.title, TagLib::String::UTF8));
        tag->setArtist(TagLib::String(track.artist, TagLib::String::UTF8));

        if (!artwork_data.empty()) {
            auto* picture = new TagLib::ID3v2::AttachedPictureFrame();
            picture->setMimeType("image/jpeg");
            picture->setType(TagLib::ID3v2::AttachedPictureFrame::FrontCover);
            picture->setPicture(TagLib::ByteVector(
                reinterpret_cast<const char*>(artwork_data.data()),
                static_cast<unsigned int>(artwork_data.size())));
            tag->addFrame(picture);
        }

        return file.save();
    }

    if (ext == ".m4a") {
        TagLib::MP4::File file(audio_file.string().c_str());
        if (!file.isValid()) return false;

        auto* tag = file.tag();
        tag->setTitle(TagLib::String(track.title, TagLib::String::UTF8));
        tag->setArtist(TagLib::String(track.artist, TagLib::String::UTF8));

        if (!artwork_data.empty()) {
            TagLib::MP4::CoverArt cover(
                TagLib::MP4::CoverArt::JPEG,
                TagLib::ByteVector(
                    reinterpret_cast<const char*>(artwork_data.data()),
                    static_cast<unsigned int>(artwork_data.size())));
            TagLib::MP4::CoverArtList covers;
            covers.append(cover);
            file.tag()->setItem("covr",
                TagLib::MP4::Item(covers));
        }

        return file.save();
    }

    return false;
}

} // namespace sc
