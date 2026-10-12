#pragma once
#include "M5GFX.h"
#include "lgfx/v1/panel/Panel_ST7789.hpp"
#include "driver/gpio.h"
#include "sdkconfig.h"

namespace adv {
// Internal Cardputer LCD uses SPI3. The external LCD and SD use SPI2.
// No framebuffer: drawing goes directly to ST7789 GRAM.
class CardTftPanel : public lgfx::LGFX_Device {
    lgfx::Bus_SPI bus_;
    lgfx::Panel_ST7789 panel_;
public:
    CardTftPanel() {
        auto bus = bus_.config();
        bus.spi_host = SPI2_HOST;
        bus.spi_mode = 0;
        bus.freq_write = 20000000;
        bus.freq_read = 8000000;
        bus.pin_sclk = 40;
        bus.pin_mosi = 14;
        bus.pin_miso = 39;
        bus.pin_dc = 6;
        bus.spi_3wire = false;
        bus.use_lock = true;
        bus_.config(bus);
        panel_.setBus(&bus_);
        auto panel = panel_.config();
        panel.pin_cs = 5;
        panel.pin_rst = 3;
        panel.pin_busy = -1;
        panel.panel_width = panel.memory_width = 240;
        panel.panel_height = panel.memory_height = 320;
        panel.offset_x = panel.offset_y = 0;
        panel.offset_rotation = 0;
        panel.readable = false;
        panel.bus_shared = true;
#ifdef CONFIG_CARDTFT_INVERT
        panel.invert = true;
#endif
#ifdef CONFIG_CARDTFT_RGB_ORDER
        panel.rgb_order = true;
#endif
        panel_.config(panel);
        setPanel(&panel_);
    }
    void begin() {
        // Keep SD deselected before any LCD SPI clocks. SD mounting is not
        // currently implemented by this SSH firmware.
        gpio_set_level(GPIO_NUM_12, 1);
        gpio_set_direction(GPIO_NUM_12, GPIO_MODE_OUTPUT);
        init();
        setRotation(1);
    }
};
}  // namespace adv
