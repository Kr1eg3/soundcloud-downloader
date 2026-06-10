#include "sc/crypto.hpp"

#include <openssl/evp.h>
#include <memory>
#include <stdexcept>

namespace sc {

std::vector<uint8_t> aes_128_cbc_decrypt(const std::vector<uint8_t>& ciphertext,
                                          const std::vector<uint8_t>& key,
                                          const std::vector<uint8_t>& iv) {
    if (key.size() != 16) {
        throw std::runtime_error("AES key must be 16 bytes");
    }
    if (iv.size() != 16) {
        throw std::runtime_error("AES IV must be 16 bytes");
    }
    if (ciphertext.empty()) {
        return {};
    }

    std::unique_ptr<EVP_CIPHER_CTX, decltype(&EVP_CIPHER_CTX_free)> ctx(
        EVP_CIPHER_CTX_new(), EVP_CIPHER_CTX_free);

    if (!ctx) {
        throw std::runtime_error("Failed to create cipher context");
    }

    if (EVP_DecryptInit_ex(ctx.get(), EVP_aes_128_cbc(), nullptr,
                           key.data(), iv.data()) != 1) {
        throw std::runtime_error("EVP_DecryptInit_ex failed");
    }

    std::vector<uint8_t> plaintext(ciphertext.size() + EVP_MAX_BLOCK_LENGTH);
    int out_len = 0;

    if (EVP_DecryptUpdate(ctx.get(), plaintext.data(), &out_len,
                          ciphertext.data(),
                          static_cast<int>(ciphertext.size())) != 1) {
        throw std::runtime_error("EVP_DecryptUpdate failed");
    }

    int final_len = 0;
    if (EVP_DecryptFinal_ex(ctx.get(), plaintext.data() + out_len, &final_len) != 1) {
        throw std::runtime_error("EVP_DecryptFinal_ex failed (bad padding or key)");
    }

    plaintext.resize(static_cast<size_t>(out_len + final_len));
    return plaintext;
}

} // namespace sc
