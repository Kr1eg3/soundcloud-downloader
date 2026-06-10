#pragma once

#include "sc/types.hpp"

#include <filesystem>

namespace sc {

class Transcoder {
public:
    static bool is_available();

    bool to_mp3(const std::filesystem::path& input,
                const std::filesystem::path& output);

    bool to_m4a(const std::filesystem::path& input,
                const std::filesystem::path& output);
};

} // namespace sc
