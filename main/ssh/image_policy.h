#pragma once
#include <string>
#include <cstdint>
#include <cstddef>
namespace adv {
constexpr size_t kPngMaxBytes = 256 * 1024;
inline bool image_command(const std::string& path, std::string& command) {
    if (path.empty() || path.size() > 512 || path[0] != '/') return false;
    command = "cat -- '";
    for (unsigned char c : path) {
        if (c < 32 || c == 127) return false;
        if (c == '\'') command += "'\\''";
        else command += static_cast<char>(c);
    }
    command += "'";
    return true;
}
inline bool png_header(const uint8_t* p, size_t n, unsigned width, unsigned height) {
    const uint8_t signature[] = {137,80,78,71,13,10,26,10};
    if (n < 33) return false;
    for (int i=0;i<8;++i) if(p[i]!=signature[i]) return false;
    auto be = [](const uint8_t* b) { return (uint32_t(b[0])<<24)|(uint32_t(b[1])<<16)|(uint32_t(b[2])<<8)|b[3]; };
    const unsigned depth=p[24], type=p[25];
    const bool valid_depth = depth==1 || depth==2 || depth==4 || depth==8;
    const bool valid_type = ((type==0 || type==3) && valid_depth) ||
                            ((type==2 || type==4 || type==6) && depth==8);
    return valid_type && be(p+8)==13 && std::string(reinterpret_cast<const char*>(p+12),4)=="IHDR" &&
        be(p+16)>0 && be(p+16)<=width && be(p+20)>0 && be(p+20)<=height &&
        p[24]<=8 && p[26]==0 && p[27]==0 && p[28]==0;
}
}
