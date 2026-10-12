#include "terminal/terminal_emulator.h"
#include <cassert>
#include <iostream>
using namespace adv;
int main() {
    TerminalEmulator t(10, 4, 4);
    t.process("A\xe6"); t.process("\x97"); t.process("\xa5");
    assert(t.cell(0,0).codepoint == 'A');
    assert(t.cell(0,1).codepoint == U'日' && t.cell(0,1).width == 2);
    assert(t.cell(0,2).continuation);
    assert(t.cursor_col() == 3);
    t.process("本語\r\nかなカナ");
    assert(t.cell(1,0).codepoint == U'か');
    assert(t.cell(1,6).codepoint == U'ナ');
    const std::string sample = "ASCII 日本語かなカナ";
    for (size_t split = 1; split < sample.size(); ++split) {
        TerminalEmulator chunks(40, 9, 0);
        chunks.process(sample.substr(0, split)); chunks.process(sample.substr(split));
        assert(chunks.cell(0,6).codepoint == U'日');
        assert(chunks.cell(0,18).codepoint == U'ナ');
    }
    t.process("\xe6"); t.reset(); t.process("日本");
    assert(t.cell(0,0).codepoint == U'日');
    t.reset(); t.process("123456789日");
    assert(t.cell(1,0).codepoint == U'日' && t.cell(1,1).continuation);
    t.process("\r\n二\r\n三\r\n四\r\n五\r\n六");
    assert(t.scrollback_size() == 3);
    assert(t.cell(3,0).codepoint == U'六');
    auto history = t.scrollback_snapshot(3);
    assert(history[0][0].codepoint == '1');
    t.process("\r\n七\r\n八");
    assert(t.scrollback_size() == 4);
    t.reset(); t.process("\x1b(0qq\x1b(B日本");
    assert(t.cell(0,0).codepoint == U'─' && t.cell(0,2).codepoint == U'日');
    t.reset(); t.process("日本\r\n\x1b[31m赤\x1b[0m");
    assert(t.cell(1,0).codepoint == U'赤' && t.cell(1,0).fg == 1);
    t.process("\x1b[?1049h別画面\x1b[?1049l");
    assert(t.cell(0,0).codepoint == U'日');
    t.resize(53,18); assert(t.cols()==53 && t.rows()==18);
    t.process("\x1b[2J\x1b[HASCII 日本語 ひらがな カタカナ ｶﾅ");
    assert(t.cell(0,6).width==2);
    t.resize(40,9); assert(t.cols()==40 && t.rows()==9);
    std::cout << "terminal: split UTF-8, CJK width, wrap, CRLF, scrollback cap, ANSI, alternate screen, resize PASS\n";
}
