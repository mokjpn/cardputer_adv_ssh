#include "ssh/host_key_store.h"
#include "mbedtls/sha256.h"
#include "mbedtls/base64.h"
#include "nvs.h"
#include <array>
#include <vector>

namespace adv {
std::string host_key_endpoint(const std::string& host, uint16_t port) {
    std::string name = host;
    for (char& c : name) if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
    return "[" + name + "]:" + std::to_string(port);
}

bool check_server_host_key(const std::string& endpoint, const char* key, size_t key_size,
                           const HostKeyConfirm& confirm, std::string& error) {
    if (!key || !key_size || endpoint.size() > 300 || endpoint.find('\n') != std::string::npos) {
        error = "Invalid server host key";
        return false;
    }
    std::array<unsigned char, 32> digest{};
    if (mbedtls_sha256(reinterpret_cast<const unsigned char*>(key), key_size, digest.data(), 0) != 0) {
        error = "Host key hash failed";
        return false;
    }
    unsigned char encoded[45]{};
    size_t written = 0;
    if (mbedtls_base64_encode(encoded, sizeof(encoded), &written, digest.data(), digest.size()) != 0) {
        error = "Host key encoding failed";
        return false;
    }
    while (written && encoded[written - 1] == '=') --written;
    std::string fingerprint = "SHA256:" + std::string(reinterpret_cast<char*>(encoded), written);
    if (mbedtls_sha256(reinterpret_cast<const unsigned char*>(endpoint.data()), endpoint.size(), digest.data(), 0) != 0) {
        error = "Host identity hash failed";
        return false;
    }
    // NVS key length <= 15. Full endpoint is also stored and compared below,
    // so a truncated index collision fails closed instead of trusting another host.
    const char* hex = "0123456789abcdef";
    std::string index = "h";
    for (size_t i = 0; i < 7; ++i) {
        index += hex[digest[i] >> 4];
        index += hex[digest[i] & 15];
    }
    nvs_handle_t handle;
    if (nvs_open("ssh_hosts", NVS_READWRITE, &handle) != ESP_OK) {
        error = "Host key store unavailable";
        return false;
    }
    auto load = [&](const std::string& identity, std::string& value) {
        size_t size = 0;
        esp_err_t rc = nvs_get_str(handle, index.c_str(), nullptr, &size);
        if (rc == ESP_ERR_NVS_NOT_FOUND) return HostKeyLookup::kMissing;
        if (rc != ESP_OK || size < 2 || size > 400) return HostKeyLookup::kError;
        std::vector<char> record(size);
        if (nvs_get_str(handle, index.c_str(), record.data(), &size) != ESP_OK) return HostKeyLookup::kError;
        std::string prefix = identity + "\n";
        std::string saved(record.data());
        if (saved.compare(0, prefix.size(), prefix) != 0) return HostKeyLookup::kError;
        value = saved.substr(prefix.size());
        return HostKeyLookup::kFound;
    };
    auto save = [&](const std::string& identity, const std::string& value) {
        const std::string record = identity + "\n" + value;
        return nvs_set_str(handle, index.c_str(), record.c_str()) == ESP_OK && nvs_commit(handle) == ESP_OK;
    };
    bool accepted = verify_host_key(endpoint, fingerprint, load, save, confirm, error);
    nvs_close(handle);
    return accepted;
}
} // namespace adv
