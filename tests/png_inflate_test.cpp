#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include "PNGdec.h"
static std::vector<uint8_t> load(const std::string& path) {
    FILE* file=fopen(path.c_str(),"rb"); assert(file); fseek(file,0,SEEK_END);
    std::vector<uint8_t> data(ftell(file)); rewind(file);
    assert(fread(data.data(),1,data.size(),file)==data.size()); fclose(file); return data;
}
static void drain(z_stream& stream,std::vector<uint8_t>& output,bool prefix) {
    int result;
    do {
        uint8_t block[513]; stream.next_out=block; stream.avail_out=sizeof(block);
        result=inflate(&stream,Z_NO_FLUSH,1);
        assert(result==Z_OK || result==Z_STREAM_END);
        output.insert(output.end(),block,block+sizeof(block)-stream.avail_out);
        if(prefix && output.size()>=40000) return;
    } while(result!=Z_STREAM_END);
}
int main(int argc,char** argv) {
    assert(argc==2); std::string dir=argv[1];
    auto compressed=load(dir+"/inflate-reference.z"), expected=load(dir+"/inflate-reference.raw");
    z_stream source{};
    source.zalloc=[](voidpf,unsigned items,unsigned size)->voidpf { return calloc(items,size); };
    source.zfree=[](voidpf,voidpf p) { free(p); };
    auto state=static_cast<inflate_state*>(calloc(1,sizeof(inflate_state))); assert(state);
    for(unsigned i=0;i<PNG_WINDOW_PAGE_COUNT;++i) {
        state->window_pages[i]=static_cast<uint8_t*>(malloc(PNG_WINDOW_PAGE_BYTES));
        assert(state->window_pages[i]);
    }
    state->window=state->window_pages[0]; source.state=reinterpret_cast<internal_state*>(state);
    assert(inflateInit(&source)==Z_OK);
    source.next_in=compressed.data(); source.avail_in=compressed.size();
    std::vector<uint8_t> first; drain(source,first,true);
    uint8_t dictionary[32768]; unsigned count=0;
    assert(inflateGetDictionary(&source,dictionary,&count)==Z_OK && count==32768);
    assert(memcmp(dictionary,first.data()+first.size()-count,count)==0);
    z_stream copy{}; assert(inflateCopy(&copy,&source)==Z_OK);
    auto second=first;
    drain(source,first,false); drain(copy,second,false);
    assert(first==expected && second==expected);
    assert(inflateEnd(&source)==Z_OK && inflateEnd(&copy)==Z_OK);
    puts("32KiB dictionary export / mid-stream copy / page and ring wrap / cleanup PASS");
    z_stream back{};
    back.zalloc=[](voidpf,unsigned n,unsigned s)->voidpf { return calloc(n,s); };
    back.zfree=[](voidpf,voidpf p) { free(p); };
    std::vector<uint8_t> window(32768), output;
    assert(inflateBackInit(&back,15,window.data())==Z_OK);
    struct Input { std::vector<uint8_t>* bytes; bool sent=false; } input{&compressed};
    auto read=[](void* arg,unsigned char** data)->unsigned {
        auto& in=*static_cast<Input*>(arg); if(in.sent) return 0; in.sent=true;
        *data=in.bytes->data()+2; return in.bytes->size()-6;
    };
    auto write=[](void* arg,unsigned char* data,unsigned n)->int {
        auto& out=*static_cast<std::vector<uint8_t>*>(arg); out.insert(out.end(),data,data+n); return 0;
    };
    assert(inflateBack(&back,read,&input,write,&output)==Z_STREAM_END);
    assert(output==expected && inflateBackEnd(&back)==Z_OK);
    puts("inflateBack caller-owned window compatibility PASS");
}
