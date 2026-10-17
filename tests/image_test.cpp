#include <cassert>
#include <cstdio>
#include <string>
#include <vector>
#include "ssh/image_policy.h"
#include "ssh/png_validate.h"
#include "PNGdec.h"
static int rows;
static int draw(PNGDRAW* p) { assert(p->iWidth==320); ++rows; return 1; }
int main(int argc,char** argv) {
    std::string command;
    assert(adv::image_command("/tmp/a'b;$(bad).png",command));
    assert(command=="cat -- '/tmp/a'\\''b;$(bad).png'");
    for (const auto& p : {"", "relative", "/bad\nname"}) assert(!adv::image_command(p,command));
    assert(!adv::image_command(std::string("/")+std::string(512,'a'),command));
    assert(argc==2);
    FILE* f=fopen(argv[1],"rb"); assert(f);
    assert(adv::validate_png(f,320,240));
    assert(!adv::validate_png(f,240,135));
    fseek(f,0,SEEK_END); std::vector<uint8_t> bytes(ftell(f)); rewind(f);
    assert(fread(bytes.data(),1,bytes.size(),f)==bytes.size()); fclose(f);
    PNG png;
    assert(png.openRAM(bytes.data(),bytes.size(),draw)==PNG_SUCCESS);
    assert(png.decode(nullptr,PNG_CHECK_CRC)==PNG_SUCCESS); assert(rows==240); png.close();
    for(size_t n=0;n<bytes.size();++n) {
        f=tmpfile(); assert(f); fwrite(bytes.data(),1,n,f);
        assert(!adv::validate_png(f,320,240)); fclose(f);
    }
    for(size_t i=0;i<bytes.size();++i) {
        bytes[i]^=1;
        f=tmpfile(); assert(f); fwrite(bytes.data(),1,bytes.size(),f);
        assert(!adv::validate_png(f,320,240)); fclose(f); bytes[i]^=1;
    }
    printf("image policy/CRC/truncation/decode passed; PNG=%zu bytes\n",sizeof(PNG));
}
