#pragma once

#include <filesystem>
#include <string>

namespace sc {

std::string sanitize_filename(const std::string& name);
bool is_soundcloud_url(const std::string& url);
bool is_playlist_url(const std::string& url);
std::filesystem::path get_cache_dir();
std::filesystem::path get_temp_path(const std::string& extension);

} // namespace sc
