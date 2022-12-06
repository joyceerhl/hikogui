// Copyright Take Vos 2021-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#include "radio_button_widget.hpp"

namespace hi::inline v1 {

box_constraints const& radio_button_widget::set_constraints(set_constraints_context const& context) noexcept
{
    _layout = {};

    // Make room for button and margin.
    _button_size = {context.theme->size, context.theme->size};
    hilet extra_size = extent2{context.theme->margin + _button_size.width(), 0.0f};
    _label_constraints = set_constraints_button(context);
    _constraints = max(_label_constraints + extra_size, _button_size);
    _constraints.set_margins(narrow_cast<int>(context.theme->margin));
    _constraints.alignment = *alignment;
    return _constraints;
}

void radio_button_widget::set_layout(widget_layout const& context) noexcept
{
    if (compare_store(_layout, context)) {
        auto alignment_ = context.left_to_right() ? *alignment : mirror(*alignment);

        if (alignment_ == horizontal_alignment::left or alignment_ == horizontal_alignment::right) {
            _button_rectangle = round(align(context.rectangle(), _button_size, alignment_));
        } else {
            hi_not_implemented();
        }

        hilet label_width = context.width() - (_button_rectangle.width() + context.theme->margin);
        if (alignment_ == horizontal_alignment::left) {
            hilet label_left = _button_rectangle.right() + context.theme->margin;
            hilet label_rectangle = aarectangle{label_left, 0.0f, label_width, narrow_cast<float>(context.height())};
            _label_shape = box_shape{_label_constraints, label_rectangle, context.theme->baseline_adjustment};

        } else if (alignment_ == horizontal_alignment::right) {
            hilet label_rectangle = aarectangle{0.0f, 0.0f, label_width, narrow_cast<float>(context.height())};
            _label_shape = box_shape{_label_constraints, label_rectangle, context.theme->baseline_adjustment};

        } else {
            hi_not_implemented();
        }

        _pip_rectangle = align(_button_rectangle, extent2{context.theme->icon_size, context.theme->icon_size}, alignment::middle_center());
    }
    set_layout_button(context);
}

void radio_button_widget::draw(draw_context const &context) noexcept
{
    if (*mode > widget_mode::invisible and overlaps(context, layout())) {
        draw_radio_button(context);
        draw_radio_pip(context);
        draw_button(context);
    }
}

void radio_button_widget::draw_radio_button(draw_context const &context) noexcept
{
    context.draw_circle(
        layout(),
        circle{_button_rectangle} * 1.02f,
        background_color(),
        focus_color(),
        layout().theme->border_width,
        border_side::inside);
}

void radio_button_widget::draw_radio_pip(draw_context const &context) noexcept
{
    _animated_value.update(state() == button_state::on ? 1.0f : 0.0f, context.display_time_point);
    if (_animated_value.is_animating()) {
        request_redraw();
    }

    // draw pip
    auto float_value = _animated_value.current_value();
    if (float_value > 0.0) {
        context.draw_circle(layout(), circle{_pip_rectangle} * 1.02f * float_value, accent_color());
    }
}

} // namespace hi::inline v1
