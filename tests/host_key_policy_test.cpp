#include "ssh/host_key_policy.h"
#include <cassert>
#include <map>
#include <iostream>
using namespace adv;
int main() {
    std::map<std::string,std::string> records;
    int prompts=0, writes=0;
    bool accept=false, write_ok=true, read_ok=true;
    auto load=[&](const std::string& ep,std::string& fp) {
        if (!read_ok) return HostKeyLookup::kError;
        if (!records.count(ep)) return HostKeyLookup::kMissing;
        fp=records[ep]; return HostKeyLookup::kFound;
    };
    auto save=[&](const std::string& ep,const std::string& fp) {
        ++writes; if (!write_ok) return false;
        records[ep]=fp; return true;
    };
    auto confirm=[&](const std::string&,const std::string&) {++prompts;return accept;};
    std::string error, key="SHA256:"+std::string(43,'a'), changed="SHA256:"+std::string(43,'b');
    auto check=[&](const std::string& ep,const std::string& fp) {return verify_host_key(ep,fp,load,save,confirm,error);};
    assert(!check("host:22",key) && writes==0);
    accept=true; assert(check("host:22",key) && writes==1 && prompts==2);
    for(int i=0;i<10;++i) assert(check("host:22",key));
    assert(prompts==2 && writes==1);
    assert(!check("host:22",changed) && prompts==2 && writes==1);
    assert(check("host:2222",changed) && writes==2);
    read_ok=false; assert(!check("host:22",key) && prompts==3);
    read_ok=true; write_ok=false; assert(!check("other:22",key));
    assert(!records.count("other:22"));
    assert(!verify_host_key("new:22",key,load,save,{},error));
    assert(!check("host:22",""));
    std::cout << "host policy: reject, accept/save, repeat, changed key, port isolation, read/write failure, no callback PASS\n";
}
