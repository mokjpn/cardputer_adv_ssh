idf_component_register(
    SRCS
        "main.cpp"
        "app/app_controller.cpp"
        "drivers/display.cpp"
        "drivers/keyboard.cpp"
        "drivers/tca8418.cpp"
        "net/wifi_manager.cpp"
        "ssh/ssh_client.cpp"
        "ssh/host_key_store.cpp"
        "storage/settings_store.cpp"
        "terminal/terminal_emulator.cpp"
        "ui/ui_app.cpp"
        "ui/text_input.cpp"
        "util/string_utils.cpp"
        "web/web_server.cpp"
    INCLUDE_DIRS
        "."
    REQUIRES
        esp_wifi
        esp_event
        nvs_flash
        esp_netif
        esp_http_server
        json
        lwip
        driver
        M5Unified
        mbedtls
        libssh2_esp
        libvterm
        spiffs
        pngdec
)

# Provision only the new, disposable PNG cache. The legacy storage partition
# intentionally has no image and must never be formatted by this feature.
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/png-cache-empty")
spiffs_create_partition_image(png_cache "${CMAKE_BINARY_DIR}/png-cache-empty" FLASH_IN_PROJECT)
