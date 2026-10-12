#include "drivers/efont_glyph.h"
#include "lgfx/Fonts/efont/lgfx_efont_ja.h"
#include <cassert>
#include <iostream>
int main() {
    size_t found=0;
    for(const uint8_t* font : {lgfx_efont_ja_10,lgfx_efont_ja_12,lgfx_efont_ja_12_b,lgfx_efont_ja_14}) {
        for(char32_t cp : U"日本語漢字ひらがなカタカナｶﾀｶﾅ全角幅。、「」〜￥") {
            if(cp) assert(adv::find_efont_glyph(font,adv::japanese_halfwidth_glyph(cp)));
        }
        for(unsigned cp=0xff61;cp<=0xff9f;++cp) assert(adv::find_efont_glyph(font,adv::japanese_halfwidth_glyph(cp)));
        // Exercise missing glyph lookup and all BMP lookup bounds under ASan.
        for(unsigned cp=0;cp<=0xffff;++cp) found+=adv::find_efont_glyph(font,cp)!=nullptr;
    }
    std::cout << "efont: four Japanese presets, kana/kanji/punctuation coverage and all BMP lookups PASS (" << found << " glyphs)\n";
}
