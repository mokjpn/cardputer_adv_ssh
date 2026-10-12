#pragma once
#include "ssh/host_key_policy.h"
#include <cstdint>

namespace adv {
// Host + port identity, independent of profile name and username.
std::string host_key_endpoint(const std::string& host, uint16_t port);
bool check_server_host_key(const std::string& endpoint, const char* key, size_t key_size,
                           const HostKeyConfirm& confirm, std::string& error);
} // namespace adv
