#include "ui/text_input.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace adv {

TextInput::TextInput(Display& display, Keyboard& keyboard) : display_(display), keyboard_(keyboard) {}

std::optional<std::string> TextInput::prompt(const std::string& title, const std::string& label,
                                             const std::string& initial, bool secret, PathHistory* history)
{
    TextInputModel model(initial, secret, history);
    display_.show_text_input(title, label, model.value(), secret, model.has_history());

    while (true) {
        auto event = keyboard_.poll();
        if (!event.has_value()) {
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }
        const auto result = model.handle(*event);
        if (result == TextInputModel::Result::kCancel) return std::nullopt;
        if (result == TextInputModel::Result::kSubmit) return model.take_value();
        display_.show_text_input(title, label, model.value(), secret, model.has_history());
    }
}

}  // namespace adv
