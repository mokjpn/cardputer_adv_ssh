#pragma once
namespace adv {
enum class KeyKind {
    kNone,
    kCharacter,
    kEnter,
    kEscape,
    kBackspace,
    kDelete,
    kTab,
    kUp,
    kDown,
    kLeft,
    kRight,
};

struct KeyEvent {
    KeyKind kind = KeyKind::kNone;
    char character = '\0';
    bool pressed = false;
};

}
