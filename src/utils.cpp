#include "sc/utils.hpp"

#include <algorithm>
#include <random>
#include <regex>
#include <sstream>

#ifdef _WIN32
#include <shlobj.h>
#include <windows.h>
#endif

namespace sc {

std::string sanitize_filename(const std::string& name) {
    std::string result = name;
    const std::string forbidden = R"(<>:"/\|?*)";
    for (auto& ch : result) {
        if (forbidden.find(ch) != std::string::npos) {
            ch = '_';
        }
    }

    while (!result.empty() && (result.back() == ' ' || result.back() == '.')) {
        result.pop_back();
    }
    while (!result.empty() && result.front() == ' ') {
        result.erase(result.begin());
    }

    if (result.size() > 200) {
        result.resize(200);
    }

    if (result.empty()) {
        result = "untitled";
    }

    return result;
}

bool is_soundcloud_url(const std::string& url) {
    std::regex pattern(R"(https?://(www\.)?(soundcloud\.com|on\.soundcloud\.com)/.+)",
                       std::regex_constants::icase);
    return std::regex_match(url, pattern);
}

bool is_playlist_url(const std::string& url) {
    return url.find("/sets/") != std::string::npos;
}

std::filesystem::path get_cache_dir() {
#ifdef _WIN32
    wchar_t* appdata = nullptr;
    if (SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &appdata) == S_OK) {
        std::filesystem::path dir = std::filesystem::path(appdata) / "sc-dl";
        CoTaskMemFree(appdata);
        std::filesystem::create_directories(dir);
        return dir;
    }
    auto dir = std::filesystem::temp_directory_path() / "sc-dl";
    std::filesystem::create_directories(dir);
    return dir;
#else
    const char* home = std::getenv("HOME");
    auto dir = std::filesystem::path(home ? home : "/tmp") / ".cache" / "sc-dl";
    std::filesystem::create_directories(dir);
    return dir;
#endif
}

std::filesystem::path get_temp_path(const std::string& extension) {
    auto tmp = std::filesystem::temp_directory_path() / "sc-dl";
    std::filesystem::create_directories(tmp);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(100000, 999999);
    auto name = "sc_" + std::to_string(dist(gen)) + extension;

    return tmp / name;
}

} // namespace sc
