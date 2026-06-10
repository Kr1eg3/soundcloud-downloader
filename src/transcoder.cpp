#include "sc/transcoder.hpp"

#include <cstdlib>
#include <string>

namespace sc {

namespace {

bool run_command(const std::string& cmd) {
    return std::system(cmd.c_str()) == 0;
}

} // namespace

bool Transcoder::is_available() {
#ifdef _WIN32
    return run_command("ffmpeg -version >NUL 2>&1");
#else
    return run_command("ffmpeg -version >/dev/null 2>&1");
#endif
}

bool Transcoder::to_mp3(const std::filesystem::path& input,
                         const std::filesystem::path& output) {
    std::string cmd = "ffmpeg -y -i \"" + input.string() +
                      "\" -codec:a libmp3lame -q:a 2 \"" + output.string() + "\"";
#ifdef _WIN32
    cmd += " >NUL 2>&1";
#else
    cmd += " >/dev/null 2>&1";
#endif
    return run_command(cmd);
}

bool Transcoder::to_m4a(const std::filesystem::path& input,
                         const std::filesystem::path& output) {
    std::string cmd = "ffmpeg -y -i \"" + input.string() +
                      "\" -codec:a copy -movflags +faststart \"" + output.string() + "\"";
#ifdef _WIN32
    cmd += " >NUL 2>&1";
#else
    cmd += " >/dev/null 2>&1";
#endif
    return run_command(cmd);
}

} // namespace sc
