#pragma once

#include "sc/types.hpp"

#include <cstdint>
#include <filesystem>
#include <vector>

namespace sc {

bool tag_file(const std::filesystem::path& audio_file,
              const TrackInfo& track,
              const std::vector<uint8_t>& artwork_data);

} // namespace sc
