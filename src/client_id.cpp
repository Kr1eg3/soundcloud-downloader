#include "sc/client_id.hpp"
#include "sc/http.hpp"
#include "sc/utils.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <stdexcept>

namespace sc {

namespace {

std::filesystem::path cache_path() {
    return get_cache_dir() / "client_id.txt";
}

std::optional<std::string> read_cached_id() {
    auto path = cache_path();
    if (!std::filesystem::exists(path)) return std::nullopt;

    auto mod_time = std::filesystem::last_write_time(path);
    auto now = std::filesystem::file_time_type::clock::now();
    auto age = std::chrono::duration_cast<std::chrono::hours>(now - mod_time);

    if (age.count() > 24) return std::nullopt;

    std::ifstream f(path);
    std::string id;
    std::getline(f, id);
    if (id.size() >= 20) return id;
    return std::nullopt;
}

void write_cached_id(const std::string& id) {
    auto path = cache_path();
    std::filesystem::create_directories(path.parent_path());
    std::ofstream f(path);
    f << id;
}

std::string scrape_client_id(HttpClient& http) {
    auto page = http.get("https://soundcloud.com");
    if (!page.ok()) {
        throw std::runtime_error("Failed to fetch SoundCloud homepage: HTTP " +
                                 std::to_string(page.status_code));
    }

    std::regex script_re(R"re(src="(https://a-v2\.sndcdn\.com/assets/[^"]+\.js)")re");
    std::vector<std::string> script_urls;
    auto begin = std::sregex_iterator(page.body.begin(), page.body.end(), script_re);
    auto end = std::sregex_iterator();
    for (auto it = begin; it != end; ++it) {
        script_urls.push_back((*it)[1].str());
    }

    if (script_urls.empty()) {
        throw std::runtime_error("No SoundCloud JS bundles found on page");
    }

    std::regex id_re(R"re(client_id\s*[:=]\s*"([a-zA-Z0-9]{32})")re");

    for (auto it = script_urls.rbegin(); it != script_urls.rend(); ++it) {
        auto js = http.get(*it);
        if (!js.ok()) continue;

        std::smatch match;
        if (std::regex_search(js.body, match, id_re)) {
            return match[1].str();
        }
    }

    throw std::runtime_error("Could not extract client_id from SoundCloud JS bundles");
}

} // namespace

std::string extract_client_id(HttpClient& http) {
    auto cached = read_cached_id();
    if (cached) return *cached;

    auto id = scrape_client_id(http);
    write_cached_id(id);
    return id;
}

void invalidate_cached_client_id() {
    auto path = cache_path();
    if (std::filesystem::exists(path)) {
        std::filesystem::remove(path);
    }
}

} // namespace sc
