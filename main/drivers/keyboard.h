#pragma once

#include <optional>
#include <string>

#include "drivers/tca8418.h"
#include "drivers/key_event.h"

namespace adv {

class Keyboard {
public:
    Keyboard();
    bool begin();
    std::optional<KeyEvent> poll();

private:
    struct RawKey {
        bool pressed = false;
        uint8_t row = 0;
        uint8_t col = 0;
    };

    RawKey decode(uint8_t raw);
    void remap(RawKey& key);
    KeyEvent convert(const RawKey& key);

    Tca8418 tca_;
    bool shift_ = false;
    bool fn_ = false;
    bool ctrl_ = false;
    bool fn_latched_ = false;
    bool ctrl_latched_ = false;
};

}  // namespace adv
