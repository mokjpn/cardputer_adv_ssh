#pragma once
#include <openssl/evp.h>
inline int mbedtls_base64_encode(unsigned char* out,size_t size,size_t* len,const unsigned char* in,size_t n) {
    if(size<4*((n+2)/3)+1) return -1;
    *len=EVP_EncodeBlock(out,in,n);return 0;
}
