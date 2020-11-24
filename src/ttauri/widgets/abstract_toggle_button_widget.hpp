// Copyright 2020 Pokitec
// All rights reserved.

#pragma once

#include "abstract_button_widget.hpp"

namespace tt {

/** An abstract toggle button widget.
 * This widgets toggles a value from the true_value to false_value and back.
 */
template<typename T>
class abstract_toggle_button_widget : public abstract_button_widget<T> {
public:
    using super = abstract_button_widget<T>;
    using value_type = T;

    value_type const false_value;

    template<typename Value = observable<value_type>>
    abstract_toggle_button_widget(
        gui_window &window,
        std::shared_ptr<widget> parent,
        value_type true_value,
        value_type false_value,
        Value &&value = {}) noexcept :
        super(window, parent, true_value, std::forward<Value>(value)),
        false_value(std::move(false_value))
    {
        _value_callback = this->value.subscribe([this](auto...) {
            this->window.request_redraw(this->window_clipping_rectangle());
        });
        _callback = this->subscribe([this]() {
            this->toggle();
        });
    }

    void toggle() noexcept
    {
        ttlet lock = std::scoped_lock(gui_system_mutex);

        if (compare_then_assign(this->value, this->value == this->false_value ? this->true_value : this->false_value)) {
            this->window.request_redraw(this->window_clipping_rectangle());
        }
    }

private:
    typename decltype(super::value)::callback_ptr_type _value_callback;
    typename super::callback_ptr_type _callback;
};

} // namespace tt