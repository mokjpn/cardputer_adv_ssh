#include "app/app_controller.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "storage/png_cache.h"

namespace adv {
namespace {
constexpr const char* TAG = "app";
}

AppController::AppController() : wifi_(store_), web_(store_), ui_(display_, keyboard_, store_, wifi_, ssh_, web_) {}

void AppController::run()
{
    display_.begin();
    display_.show_status("Boot", {"Initializing storage..."});
    if (store_.begin() != ESP_OK) {
        display_.show_status("Storage error", {"SSH host keys unavailable", "NVS was NOT erased", "Recover storage explicitly"});
        ESP_LOGE(TAG, "storage initialization failed; refusing to erase host keys");
        return;
    }

    // Verify only the dedicated cache. No automatic repair or formatting.
    // Unmount before Wi-Fi/SSH so the check does not retain filesystem buffers.
    esp_vfs_spiffs_conf_t cache{};
    cache.base_path = "/images";
    cache.partition_label = kPngCacheLabel;
    cache.max_files = 1;
    cache.format_if_mount_failed = false;
    esp_err_t cache_result = esp_vfs_spiffs_register(&cache);
    if (cache_result == ESP_OK) {
        size_t total = 0, used = 0;
        esp_err_t info = esp_spiffs_info(kPngCacheLabel, &total, &used);
        ESP_LOGI(TAG, "PNG cache: %s total=%u used=%u", esp_err_to_name(info),
                 static_cast<unsigned>(total), static_cast<unsigned>(used));
        esp_vfs_spiffs_unregister(kPngCacheLabel);
    } else {
        ESP_LOGW(TAG, "PNG cache: %s; existing storage untouched", esp_err_to_name(cache_result));
    }

    display_.show_status("Boot", {"Initializing keyboard..."});
    if (!keyboard_.begin()) {
        display_.show_status("Boot", {"Keyboard init failed", "Serial monitor only"});
        ESP_LOGW(TAG, "keyboard init failed");
    }

    display_.show_status("Boot", {"Starting Wi-Fi..."});
    ESP_ERROR_CHECK(wifi_.begin());

    display_.show_status("Boot", {"Trying saved Wi-Fi..."});
    wifi_.auto_connect();

    ui_.run();
}

}  // namespace adv
