#pragma once
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include "ssh/image_policy.h"

namespace adv {
struct PngProbe {
    size_t bytes = 0;
    uint32_t crc32 = 0;
    std::array<uint8_t, 33> header{};
    std::array<uint8_t, 12> tail{};
};

// Bounded, streaming diagnostics; no image-sized allocation or file changes.
inline bool probe_png(FILE* file, PngProbe& result) {
    result = {};
    if (fseek(file, 0, SEEK_SET) != 0) return false;
    uint8_t buffer[512];
    uint32_t crc = ~0u;
    size_t n;
    while ((n = fread(buffer, 1, sizeof(buffer), file)) != 0) {
        if (result.bytes + n > kPngMaxBytes) return false;
        if (result.bytes < result.header.size()) {
            const size_t count = std::min(n, result.header.size()-result.bytes);
            memcpy(result.header.data()+result.bytes, buffer, count);
        }
        if (n >= result.tail.size()) {
            memcpy(result.tail.data(), buffer+n-result.tail.size(), result.tail.size());
        } else {
            memmove(result.tail.data(), result.tail.data()+n, result.tail.size()-n);
            memcpy(result.tail.data()+result.tail.size()-n, buffer, n);
        }
        for (size_t i = 0; i < n; ++i) {
            crc ^= buffer[i];
            for (int bit=0; bit<8; ++bit)
                crc = (crc >> 1) ^ (0xedb88320u & (0u-(crc & 1)));
        }
        result.bytes += n;
    }
    result.crc32 = ~crc;
    const bool ok = !ferror(file);
    return fseek(file, 0, SEEK_SET) == 0 && ok;
}
}
