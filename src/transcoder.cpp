#include "sc/transcoder.hpp"

#include <array>
#include <cstdio>
#include <stdexcept>

#ifdef _WIN32
#include <windows.h>
#else
#include <cstdlib>
#endif

namespace sc {

namespace {

#ifdef _WIN32
bool run_command(const std::string& cmd) {
    STARTUPINFOA si = {};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi = {};

    std::string mutable_cmd = cmd;

    if (!CreateProcessA(nullptr, mutable_cmd.data(), nullptr, nullptr,
                        FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        return false;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exit_code = 1;
    GetExitCodeProcess(pi.hProcess, &exit_code);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return exit_code == 0;
}
#else
bool run_command(const std::string& cmd) {
    return std::system(cmd.c_str()) == 0;
}
#endif

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
