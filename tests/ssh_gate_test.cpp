#include "ssh/ssh_client.h"
#include "ssh/host_key_store.h"
#include "nvs.h"
#include <cassert>
#include <cstring>
#include <map>
#include <iostream>
#include <sys/socket.h>
using namespace adv;
namespace {
std::map<std::string,std::string> records, pending;
bool storage_ok=true, commit_ok=true, user_accept=false, empty_key=false;
int auths=0, prompts=0;
std::string server_key="abc", shown;
}
// No network connection or SSH server is used. The production connect method
// runs against a mocked transport/libssh2 and in-memory NVS, with real SHA256.
extern "C" int __wrap_connect(int,const sockaddr*,socklen_t) {return 0;}
esp_err_t nvs_open(const char*,int,nvs_handle_t* h) {*h=1;return storage_ok?0:-1;}
esp_err_t nvs_get_str(nvs_handle_t,const char* key,char* out,size_t* n) {
    if(!records.count(key)) return ESP_ERR_NVS_NOT_FOUND;
    const auto& s=records[key];
    if(!out) {*n=s.size()+1;return 0;}
    if(*n<s.size()+1) return -1;
    std::memcpy(out,s.c_str(),s.size()+1);return 0;
}
esp_err_t nvs_set_str(nvs_handle_t,const char* k,const char* v) {pending[k]=v;return 0;}
esp_err_t nvs_commit(nvs_handle_t) {if(!commit_ok)return -1;records.insert(pending.begin(),pending.end());pending.clear();return 0;}
void nvs_close(nvs_handle_t) {pending.clear();}
extern "C" {
int libssh2_init(int) {return 0;}
void libssh2_exit() {}
LIBSSH2_SESSION* libssh2_session_init_ex(LIBSSH2_ALLOC_FUNC((*)),LIBSSH2_FREE_FUNC((*)),LIBSSH2_REALLOC_FUNC((*)),void*) {return reinterpret_cast<LIBSSH2_SESSION*>(1);}
void libssh2_session_set_blocking(LIBSSH2_SESSION*,int) {}
int libssh2_session_handshake(LIBSSH2_SESSION*,libssh2_socket_t) {return 0;}
const char* libssh2_session_hostkey(LIBSSH2_SESSION*,size_t* size,int* type) {*size=server_key.size();*type=1;return empty_key?nullptr:server_key.data();}
int libssh2_session_last_error(LIBSSH2_SESSION*,char**,int*,int) {return 0;}
int libssh2_session_disconnect_ex(LIBSSH2_SESSION*,int,const char*,const char*) {return 0;}
int libssh2_session_free(LIBSSH2_SESSION*) {return 0;}
int libssh2_channel_close(LIBSSH2_CHANNEL*) {return 0;}
int libssh2_channel_free(LIBSSH2_CHANNEL*) {return 0;}
int libssh2_userauth_password_ex(LIBSSH2_SESSION*,const char*,unsigned,const char*,unsigned,LIBSSH2_PASSWD_CHANGEREQ_FUNC((*))) {++auths;return 0;}
int libssh2_userauth_publickey_frommemory(LIBSSH2_SESSION*,const char*,size_t,const char*,size_t,const char*,size_t,const char*) {++auths;return 0;}
}
int main() {
    SshClient client;
    SshProfile profile;profile.host="127.0.0.1";profile.username="test";profile.password="synthetic";
    auto confirm=[&](const std::string&,const std::string& fp) {++prompts;shown=fp;assert(auths==0);return user_accept;};
    assert(client.connect(profile,"synthetic-key",confirm)!=0 && auths==0);
    assert(shown=="SHA256:ungWv48Bz+pBQUDeXa4iI7ADYaOWF3qctBD/YfIAFa0");
    user_accept=true;assert(client.connect(profile,"synthetic-key",confirm)==0 && auths==1);
    for(int i=0;i<5;++i) assert(client.connect(profile,"")==0);
    assert(auths==6 && prompts==2);
    server_key="changed";
    assert(client.connect(profile,"synthetic-key",confirm)!=0 && auths==6 && prompts==2);
    assert(client.last_error()=="HOST KEY CHANGED - refused");
    assert(!client.connected());
    server_key="abc";profile.name="renamed";profile.username="other";
    assert(client.connect(profile,"")==0 && auths==7);
    storage_ok=false;assert(client.connect(profile,"")!=0 && auths==7);
    storage_ok=true;profile.port=2222;
    assert(client.connect(profile,"")!=0 && auths==7); // missing callback
    auto approve=[](const std::string&,const std::string&){return true;};
    commit_ok=false;assert(client.connect(profile,"",approve)!=0 && auths==7);
    commit_ok=true;assert(client.connect(profile,"",approve)==0 && auths==8);
    empty_key=true;assert(client.connect(profile,"",approve)!=0 && auths==8);
    empty_key=false;
    for(auto& item:records) item.second="corrupt";
    assert(client.connect(profile,"",approve)!=0 && auths==8);
    assert(host_key_endpoint("EXAMPLE.COM",22)=="[example.com]:22");
    std::cout << "SSH auth gate + NVS adapter: fingerprint vector, reject before either auth, reconnect, changed key, renamed profile, port, storage error/corruption PASS\n";
}
