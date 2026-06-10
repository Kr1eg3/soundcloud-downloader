cmake_minimum_required(VERSION 3.20)
project(sc_downloader VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(CLI11 CONFIG REQUIRED)
find_package(TagLib CONFIG REQUIRED)

add_executable(sc-dl
    src/main.cpp
    src/http.cpp
    src/utils.cpp
    src/client_id.cpp
    src/api.cpp
    src/hls.cpp
    src/crypto.cpp
    src/transcoder.cpp
    src/tagger.cpp
    src/downloader.cpp
    src/progress.cpp
)

target_include_directories(sc-dl PRIVATE include)

target_link_libraries(sc-dl PRIVATE
    CURL::libcurl
    OpenSSL::SSL
    OpenSSL::Crypto
    nlohmann_json::nlohmann_json
    CLI11::CLI11
    TagLib::tag
)

if(WIN32)
    target_link_libraries(sc-dl PRIVATE ws2_32 crypt32)
endif()
