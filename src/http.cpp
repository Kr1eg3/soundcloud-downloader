#include "sc/http.hpp"

#include <curl/curl.h>
#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <thread>

namespace sc {

namespace {

size_t write_string_callback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* str = static_cast<std::string*>(userdata);
    str->append(ptr, size * nmemb);
    return size * nmemb;
}

size_t write_binary_callback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* vec = static_cast<std::vector<uint8_t>*>(userdata);
    auto bytes = size * nmemb;
    vec->insert(vec->end(), ptr, ptr + bytes);
    return bytes;
}

struct FileWriteData {
    std::ofstream stream;
    ProgressCallback progress;
    size_t downloaded = 0;
};

size_t write_file_callback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* data = static_cast<FileWriteData*>(userdata);
    auto bytes = size * nmemb;
    data->stream.write(ptr, static_cast<std::streamsize>(bytes));
    data->downloaded += bytes;
    return bytes;
}

int progress_function(void* clientp, curl_off_t dltotal, curl_off_t dlnow,
                      curl_off_t /*ultotal*/, curl_off_t /*ulnow*/) {
    auto* data = static_cast<FileWriteData*>(clientp);
    if (data->progress && dltotal > 0) {
        data->progress(static_cast<size_t>(dlnow), static_cast<size_t>(dltotal));
    }
    return 0;
}

} // namespace

struct HttpClient::Impl {
    Impl() {
        curl_global_init(CURL_GLOBAL_DEFAULT);
    }
    ~Impl() {
        curl_global_cleanup();
    }
};

HttpClient::HttpClient() : impl_(std::make_unique<Impl>()) {}
HttpClient::~HttpClient() = default;

HttpResponse HttpClient::get(const std::string& url) {
    const int max_retries = 3;

    for (int attempt = 0; attempt < max_retries; ++attempt) {
        std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curl(
            curl_easy_init(), curl_easy_cleanup);

        if (!curl) {
            return {0, "", "Failed to initialize curl"};
        }

        HttpResponse response;

        curl_easy_setopt(curl.get(), CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, write_string_callback);
        curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &response.body);
        curl_easy_setopt(curl.get(), CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl.get(), CURLOPT_MAXREDIRS, 10L);
        curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT, 30L);
        curl_easy_setopt(curl.get(), CURLOPT_USERAGENT,
            "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 "
            "(KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36");
        curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYPEER, 1L);

        CURLcode res = curl_easy_perform(curl.get());

        if (res == CURLE_OK) {
            curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &response.status_code);
            if (response.status_code == 429 && attempt < max_retries - 1) {
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(1000 * (attempt + 1)));
                continue;
            }
            return response;
        }

        response.error = curl_easy_strerror(res);
        if (attempt < max_retries - 1) {
            std::this_thread::sleep_for(
                std::chrono::milliseconds(500 * (attempt + 1)));
            continue;
        }
        return response;
    }

    return {0, "", "Max retries exceeded"};
}

std::vector<uint8_t> HttpClient::get_binary(const std::string& url) {
    std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curl(
        curl_easy_init(), curl_easy_cleanup);

    if (!curl) {
        throw std::runtime_error("Failed to initialize curl");
    }

    std::vector<uint8_t> data;

    curl_easy_setopt(curl.get(), CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, write_binary_callback);
    curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &data);
    curl_easy_setopt(curl.get(), CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl.get(), CURLOPT_MAXREDIRS, 10L);
    curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT, 60L);
    curl_easy_setopt(curl.get(), CURLOPT_USERAGENT,
        "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 "
        "(KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36");

    CURLcode res = curl_easy_perform(curl.get());
    if (res != CURLE_OK) {
        throw std::runtime_error(std::string("HTTP request failed: ") +
                                 curl_easy_strerror(res));
    }

    long status;
    curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &status);
    if (status < 200 || status >= 300) {
        throw std::runtime_error("HTTP " + std::to_string(status) + " for " + url);
    }

    return data;
}

bool HttpClient::download_to_file(const std::string& url,
                                   const std::filesystem::path& path,
                                   ProgressCallback progress) {
    std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curl(
        curl_easy_init(), curl_easy_cleanup);

    if (!curl) return false;

    FileWriteData file_data;
    file_data.stream.open(path, std::ios::binary);
    if (!file_data.stream) return false;
    file_data.progress = std::move(progress);

    curl_easy_setopt(curl.get(), CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, write_file_callback);
    curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &file_data);
    curl_easy_setopt(curl.get(), CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl.get(), CURLOPT_MAXREDIRS, 10L);
    curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT, 300L);
    curl_easy_setopt(curl.get(), CURLOPT_USERAGENT,
        "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 "
        "(KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36");

    if (file_data.progress) {
        curl_easy_setopt(curl.get(), CURLOPT_XFERINFOFUNCTION, progress_function);
        curl_easy_setopt(curl.get(), CURLOPT_XFERINFODATA, &file_data);
        curl_easy_setopt(curl.get(), CURLOPT_NOPROGRESS, 0L);
    }

    CURLcode res = curl_easy_perform(curl.get());
    file_data.stream.close();

    if (res != CURLE_OK) {
        std::filesystem::remove(path);
        return false;
    }

    long status;
    curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &status);
    if (status < 200 || status >= 300) {
        std::filesystem::remove(path);
        return false;
    }

    return true;
}

} // namespace sc
