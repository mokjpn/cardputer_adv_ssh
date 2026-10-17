#define main gate_regression_main
#include "ssh_gate_test.cpp"
#undef main
#include "ssh/image_policy.h"
#include "freertos/task.h"
static std::string payload;
static size_t offset;
static bool stalled;
static int status_code;
extern "C" {
LIBSSH2_CHANNEL* libssh2_channel_open_ex(LIBSSH2_SESSION*,const char*,unsigned,unsigned,unsigned,const char*,unsigned) {return reinterpret_cast<LIBSSH2_CHANNEL*>(2);}
int libssh2_session_last_errno(LIBSSH2_SESSION*) {return LIBSSH2_ERROR_EAGAIN;}
int libssh2_channel_process_startup(LIBSSH2_CHANNEL*,const char*,unsigned,const char* c,unsigned n) {
    assert(std::string(c,n)=="cat -- '/tmp/image.png'"); return 0;
}
ssize_t libssh2_channel_read_ex(LIBSSH2_CHANNEL*,int stream,char* b,size_t n) {
    if(stalled) return LIBSSH2_ERROR_EAGAIN;
    if(stream) return 0;
    n=std::min(n,payload.size()-offset); memcpy(b,payload.data()+offset,n);offset+=n;return n;
}
int libssh2_channel_eof(LIBSSH2_CHANNEL*) {return !stalled && offset==payload.size();}
int libssh2_channel_get_exit_status(LIBSSH2_CHANNEL*) {return status_code;}
}
int main() {
    SshClient client; SshProfile profile; profile.host="127.0.0.1"; profile.username="test"; profile.password="synthetic";
    auto approve=[](const std::string&,const std::string&){return true;};
    auto run=[&](size_t size,bool stall,bool cancelled,int status,bool expected) {
        assert(client.connect(profile,"",approve)==0);
        payload.resize(size); for(size_t i=0;i<size;++i) payload[i]=char(i%256); offset=0;stalled=stall;status_code=status;test_ticks=0;
        FILE* file=tmpfile(); assert(file);
        bool ok=client.fetch_png("/tmp/image.png",file,[&](){return cancelled;})==ESP_OK;
        assert(ok==expected); assert(client.connected()==expected);
        if(ok) {
            assert(ftell(file)==long(size)); rewind(file);
            for(size_t i=0;i<size;++i) assert(fgetc(file)==static_cast<unsigned char>(payload[i]));
        }
        fclose(file);
    };
    run(1025,false,false,0,true);
    run(adv::kPngMaxBytes,false,false,0,true);
    run(adv::kPngMaxBytes+1,false,false,0,false);
    run(0,false,false,0,false);
    run(10,false,false,1,false);
    run(10,false,true,0,false);
    run(10,true,false,0,false);
    run(10,false,false,0,true);
    std::cout<<"bounded exec: binary stream, exact limit, oversize, empty, exit failure, cancel, timeout, reconnect PASS\n";
}
