#include <cassert>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>
#include "PNGdec.h"
#include "ssh/png_validate.h"
#include "png_file_callbacks.inl"
struct State { PNG* png; int rows=0; uint32_t hash=2166136261u; };
static int draw(PNGDRAW* line) {
    auto& state=*static_cast<State*>(line->pUser);
    assert(line->iWidth==320 && line->y==state.rows);
    uint16_t pixels[320]; state.png->getLineAsRGB565(line,pixels,PNG_RGB565_LITTLE_ENDIAN,0);
    for (auto pixel:pixels) {
        state.hash=(state.hash^(pixel&255))*16777619u;
        state.hash=(state.hash^(pixel>>8))*16777619u;
    }
    ++state.rows; return 1;
}
int main(int argc,char** argv) {
    assert(argc==2); std::string directory=argv[1];
    FILE* manifest=fopen((directory+"/manifest.tsv").c_str(),"r"); assert(manifest);
    char name[128]; unsigned expected; int cases=0;
    while (fscanf(manifest,"%127s %x",name,&expected)==2) {
        auto path=directory+"/"+name;
        FILE* file=fopen(path.c_str(),"rb"); assert(file);
        assert(adv::validate_png(file,320,240));
        fseek(file,0,SEEK_END); std::vector<uint8_t> bytes(ftell(file)); rewind(file);
        assert(fread(bytes.data(),1,bytes.size(),file)==bytes.size()); fclose(file);
        for(int mode=0;mode<2;++mode) {
            auto png=std::make_unique<PNG>(); assert(png->workspaceReady()); State state{png.get()};
            int opened=mode==0 ? png->openRAM(bytes.data(),bytes.size(),draw)
                : png->open(path.c_str(),png_open,png_close,png_read,png_seek,draw);
            assert(opened==PNG_SUCCESS);
            int result=png->decode(&state,PNG_CHECK_CRC); png->close();
            if(result!=PNG_SUCCESS || state.rows!=240 || state.hash!=expected) {
                printf("FAIL %s mode=%d result=%d rows=%d hash=%08x expected=%08x\n",name,mode,result,state.rows,state.hash,expected); fflush(stdout);
            }
            assert(result==PNG_SUCCESS && state.rows==240 && state.hash==expected);
        }
        ++cases; printf("%s RAM/FILE/reference RGB565 PASS\n",name); fflush(stdout);
    }
    fclose(manifest); assert(cases==13);
    printf("%d corpus cases / 26 decodes PASS; object=%zu extra=%zu maxblock=%zu\n",cases,sizeof(PNG),PNG::extraWorkspaceBytes(),PNG::largestAllocationBytes());
    manifest=fopen((directory+"/negative.txt").c_str(),"r"); assert(manifest); cases=0;
    while(fscanf(manifest,"%127s",name)==1) {
        auto path=directory+"/"+name;
        FILE* file=fopen(path.c_str(),"rb"); assert(file);
        // PNG structure/CRCs are valid; the compressed stream itself is bad.
        assert(adv::validate_png(file,320,240)); fclose(file);
        auto png=std::make_unique<PNG>(); State state{png.get()};
        assert(png->open(path.c_str(),png_open,png_close,png_read,png_seek,draw)==PNG_SUCCESS);
        const int result=png->decode(&state,PNG_CHECK_CRC); png->close();
        assert(result!=PNG_SUCCESS); ++cases;
    }
    fclose(manifest); assert(cases>=5);
    printf("%d malformed DEFLATE streams with valid PNG CRC rejected PASS\n",cases);
}
