#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <unordered_map>
#include <vector>
#include "PNGdec.h"
static std::unordered_map<void*,size_t> live;
static int calls, fail_at;
static size_t budget, largest_request;
extern "C" void* png_test_alloc(size_t bytes) {
    ++calls;
    if (bytes>largest_request) largest_request=bytes;
    if (calls==fail_at || bytes>31744 || bytes>budget) return nullptr;
    void* p=malloc(bytes); assert(p); live[p]=bytes; budget-=bytes; return p;
}
extern "C" void png_test_free(void* p) {
    if (!p) return;
    auto entry=live.find(p); assert(entry!=live.end()); budget+=entry->second; live.erase(entry); free(p);
}
static int draw(PNGDRAW*) { return 1; }
int main(int argc,char** argv) {
    assert(argc==2);
    FILE* file=fopen(argv[1],"rb"); assert(file); fseek(file,0,SEEK_END);
    std::vector<uint8_t> data(ftell(file)); rewind(file);
    assert(fread(data.data(),1,data.size(),file)==data.size()); fclose(file);
    assert(sizeof(PNG)<31744 && PNG::largestAllocationBytes()<31744);
    for(int failure=0; failure<=4; ++failure) {
        fail_at=failure; calls=0; largest_request=0;
        budget=84212-sizeof(PNG);
        {
            PNG png;
            assert(png.workspaceReady()==(failure==0));
            assert(png.openRAM(data.data(),data.size(),draw)==PNG_SUCCESS);
            int result=png.decode(nullptr,PNG_CHECK_CRC); png.close();
            assert(result==(failure==0 ? PNG_SUCCESS : PNG_MEM_ERROR));
            if(failure==0) assert(budget>=32768);
        }
        assert(live.empty() && budget==84212-sizeof(PNG));
        assert(largest_request==8192);
        printf("allocation failure=%d cleanup PASS\n",failure);
    }
    printf("84212 free / 31744 maximum block model PASS; object=%zu dictionary=4x8192\n",sizeof(PNG));
}
