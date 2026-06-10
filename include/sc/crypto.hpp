#pragma once

#include <cstdint>
#include <vector>

namespace sc {

std::vector<uint8_t> aes_128_cbc_decrypt(const std::vector<uint8_t>& ciphertext,
                                          const std::vector<uint8_t>& key,
                                          const std::vector<uint8_t>& iv);

} // namespace sc
