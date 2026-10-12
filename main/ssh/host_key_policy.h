#pragma once
#include <functional>
#include <string>

namespace adv {
enum class HostKeyLookup { kMissing, kFound, kError };
using HostKeyConfirm = std::function<bool(const std::string&, const std::string&)>;
using HostKeyLoad = std::function<HostKeyLookup(const std::string&, std::string&)>;
using HostKeySave = std::function<bool(const std::string&, const std::string&)>;

// Pure policy shared by firmware and host tests. A changed key can never be
// accepted through the first-use confirmation path. Storage errors fail closed.
inline bool verify_host_key(const std::string& endpoint, const std::string& fingerprint,
                            const HostKeyLoad& load, const HostKeySave& save,
                            const HostKeyConfirm& confirm, std::string& error) {
    if (endpoint.empty() || fingerprint.size() != 50 || fingerprint.compare(0, 7, "SHA256:") != 0) {
        error = "Invalid host key";
        return false;
    }
    std::string stored;
    switch (load(endpoint, stored)) {
        case HostKeyLookup::kError:
            error = "Host key store error";
            return false;
        case HostKeyLookup::kFound:
            if (stored == fingerprint) return true;
            error = "HOST KEY CHANGED - refused";
            return false;
        case HostKeyLookup::kMissing:
            if (!confirm || !confirm(endpoint, fingerprint)) {
                error = "Host key not accepted";
                return false;
            }
            if (!save(endpoint, fingerprint)) {
                error = "Host key save failed";
                return false;
            }
            return true;
    }
    error = "Host key lookup failed";
    return false;
}
} // namespace adv
