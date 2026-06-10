#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace sc {

struct HttpResponse {
    long status_code = 0;
    std::string body;
    std::string error;
    bool ok() const { return status_code >= 200 && status_code < 300; }
};

using ProgressCallback = std::function<void(size_t downloaded, size_t total)>;

class HttpClient {
public:
    HttpClient();
    ~HttpClient();

    HttpClient(const HttpClient&) = delete;
    HttpClient& operator=(const HttpClient&) = delete;

    HttpResponse get(const std::string& url);
    std::vector<uint8_t> get_binary(const std::string& url);
    bool download_to_file(const std::string& url,
                          const std::filesystem::path& path,
                          ProgressCallback progress = nullptr);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace sc
