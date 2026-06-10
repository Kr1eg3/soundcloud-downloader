#pragma once

#include <optional>
#include <string>

namespace sc {

class HttpClient;

std::string extract_client_id(HttpClient& http);
void invalidate_cached_client_id();

} // namespace sc
