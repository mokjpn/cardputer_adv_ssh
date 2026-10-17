#pragma once
#include <cstdio>
#include <cstdint>
#include <cstring>
#include "ssh/image_policy.h"
namespace adv {
// Streaming structural/CRC validation before exposing any pixels to decoder.
inline bool validate_png(FILE* f, unsigned width, unsigned height) {
    uint8_t h[33];
    rewind(f);
    if (fread(h,1,33,f)!=33 || !png_header(h,33,width,height)) return false;
    fseek(f,8,SEEK_SET);
    size_t total=8;
    bool ihdr=false, idat=false, ended_data=false, palette=false;
    while (true) {
        uint8_t head[8];
        if(fread(head,1,8,f)!=8) return false;
        uint32_t len=(uint32_t(head[0])<<24)|(uint32_t(head[1])<<16)|(uint32_t(head[2])<<8)|head[3];
        if(len>kPngMaxBytes || total+12+len>kPngMaxBytes) return false;
        const bool first = !ihdr;
        bool is_header=memcmp(head+4,"IHDR",4)==0, data=memcmp(head+4,"IDAT",4)==0;
        bool end=memcmp(head+4,"IEND",4)==0, pal=memcmp(head+4,"PLTE",4)==0;
        if(first ? (!is_header || len!=13) : is_header) return false;
        if(data && ended_data) return false;
        if(idat && !data) ended_data=true;
        if(pal && (palette || idat || !len || len>768 || len%3)) return false;
        if(!is_header && !data && !end && !pal && !(head[4]&32)) return false;
        if(end && (len || !idat)) return false;
        if(data && h[25]==3 && !palette) return false;
        uint32_t crc=~0u;
        auto update=[&](uint8_t c) {
            crc ^= c;
            for(int bit=0;bit<8;++bit) crc=(crc>>1) ^ (0xedb88320u & (0u-(crc&1)));
        };
        for(int i=4;i<8;++i) update(head[i]);
        for(uint32_t i=0;i<len;++i) { int c=fgetc(f); if(c==EOF) return false; update(c); }
        uint8_t check[4];
        if(fread(check,1,4,f)!=4) return false;
        uint32_t expected=(uint32_t(check[0])<<24)|(uint32_t(check[1])<<16)|(uint32_t(check[2])<<8)|check[3];
        if((~crc)!=expected) return false;
        total+=12+len; ihdr=true; idat |= data; palette |= pal;
        if(end) return fgetc(f)==EOF && !ferror(f);
    }
}
}
