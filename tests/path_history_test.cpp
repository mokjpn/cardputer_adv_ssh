#include "ui/path_history.h"
#include <cassert>
#include <iostream>
using namespace adv;
KeyEvent ch(char c) { return {KeyKind::kCharacter,c,true}; }
KeyEvent key(KeyKind k) { return {k,0,true}; }
int main() {
 PathHistory h;
 TextInputModel empty("",false,&h);
 empty.handle(ch(16)); empty.handle(ch(14)); assert(empty.value().empty());
 assert(empty.handle(key(KeyKind::kEnter))==TextInputModel::Result::kSubmit); assert(h.size()==0);
 h.remember("/a.png");
 {TextInputModel m("",false,&h); assert(m.value()=="/a.png"); m.handle(key(KeyKind::kEnter));} assert(h.size()==1);
 {TextInputModel m("",false,&h); m.handle(ch('x')); assert(m.handle(key(KeyKind::kEscape))==TextInputModel::Result::kCancel);}
 {TextInputModel m("",false,&h); assert(m.value()=="/a.png");}
 h.remember("/b.png"); h.remember("/c.png");
 TextInputModel m("",false,&h); assert(m.value()=="/c.png");
 m.handle(ch(16)); assert(m.value()=="/b.png"); m.handle(ch(16)); assert(m.value()=="/a.png");
 for(int i=0;i<20;++i)m.handle(ch(16)); assert(m.value()=="/a.png");
 m.handle(ch(14)); assert(m.value()=="/b.png"); m.handle(ch(14)); assert(m.value()=="/c.png");
 m.handle(ch('X')); auto draft=m.value(); m.handle(ch(16)); assert(m.value()=="/c.png");
 m.handle(ch(16)); assert(m.value()=="/b.png"); m.handle(ch(14)); m.handle(ch(14)); assert(m.value()==draft);
 for(int i=0;i<20;++i)m.handle(ch(14)); assert(m.value()==draft);
 m.handle(ch(16)); m.handle(key(KeyKind::kBackspace)); assert(h.at(0)=="/c.png");
 m.handle(ch(16)); m.handle(ch(14)); assert(m.value()=="/c.pn");
 TextInputModel secret("",true,&h); for(int i=0;i<100;++i)secret.handle(ch('s')); assert(secret.value().size()==96); secret.handle(key(KeyKind::kEnter)); assert(h.size()==3);
 PathHistory bounded; for(int i=0;i<10000;++i){ bounded.remember("/"+std::to_string(i)+std::string(i%510,'a')); assert(bounded.size()<=8); assert(bounded.storage_bytes()<=1024); }
 PathHistory count; for(int i=0;i<10;++i)count.remember("/"+std::to_string(i)); assert(count.size()==8); assert(count.at(7)=="/2");
 TextInputModel longpath("",false,&bounded); for(int i=0;i<600;++i)longpath.handle(ch('x')); assert(longpath.value().size()==512);
 bounded.remember(std::string(513,'z')); assert(bounded.storage_bytes()<=1024);
 std::cout<<"path history: reopen, duplicates, cancel, empty, navigation, drafts, secret isolation, 10000 bounded submissions passed\n";
}
