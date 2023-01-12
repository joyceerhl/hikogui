// Copyright Take Vos 2021-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#include "selection_widget.hpp"
#include "../text/font_book.hpp"
#include "../GFX/pipeline_SDF_device_shared.hpp"
#include "../loop.hpp"

namespace hi::inline v1 {

selection_widget::~selection_widget()
{
    hi_assert_not_null(delegate);
    delegate->deinit(*this);
}

selection_widget::selection_widget(widget *parent, std::shared_ptr<delegate_type> delegate) noexcept :
    super(parent), delegate(std::move(delegate))
{
    hi_assert_not_null(this->delegate);

    _current_label_widget = std::make_shared<label_widget>(this, alignment, text_style);
    _current_label_widget->mode = widget_mode::invisible;
    _off_label_widget = std::make_shared<label_widget>(this, off_label, alignment, semantic_text_style::placeholder);

    _overlay_widget = std::make_shared<overlay_widget>(this);
    _overlay_widget->mode = widget_mode::invisible;
    _scroll_widget = &_overlay_widget->make_widget<vertical_scroll_widget>();
    _column_widget = &_scroll_widget->make_widget<column_widget>();

    _off_label_cbt = this->off_label.subscribe([&](auto...) {
        ++global_counter<"selection_widget:off_label:constrain">;
        process_event({gui_event_type::window_reconstrain});
    });

    _delegate_cbt = this->delegate->subscribe([&] {
        _notification_from_delegate = true;
        ++global_counter<"selection_widget:delegate:constrain">;
        process_event({gui_event_type::window_reconstrain});
    });

    this->delegate->init(*this);
}

[[nodiscard]] generator<widget *> selection_widget::children() const noexcept
{
    co_yield _overlay_widget.get();
    co_yield _current_label_widget.get();
    co_yield _off_label_widget.get();
}

[[nodiscard]] box_constraints selection_widget::update_constraints() noexcept
{
    hi_assert_not_null(_off_label_widget);
    hi_assert_not_null(_current_label_widget);
    hi_assert_not_null(_overlay_widget);

    if (_notification_from_delegate.exchange(false)) {
        repopulate_options();
    }

    _layout = {};
    _off_label_constraints = _off_label_widget->update_constraints();
    _current_label_constraints = _current_label_widget->update_constraints();
    _overlay_constraints = _overlay_widget->update_constraints();

    hilet extra_size = extent2i{theme().size() + theme().margin<int>() * 2, theme().margin<int>() * 2};

    auto r = max(_off_label_constraints + extra_size, _current_label_constraints + extra_size);

    // Make it so that the scroll widget can scroll vertically.
    _scroll_widget->minimum.copy()->height() = theme().size();

    r.minimum.width() = std::max(r.minimum.width(), _overlay_constraints.minimum.width() + extra_size.width());
    r.preferred.width() = std::max(r.preferred.width(), _overlay_constraints.preferred.width() + extra_size.width());
    r.maximum.width() = std::max(r.maximum.width(), _overlay_constraints.maximum.width() + extra_size.width());
    r.margins = theme().margin();
    r.padding = theme().margin();
    r.alignment = resolve(*alignment, os_settings::left_to_right());
    hi_axiom(r.holds_invariant());
    return r;
}

void selection_widget::set_layout(widget_layout const& context) noexcept
{
    if (compare_store(_layout, context)) {
        if (os_settings::left_to_right()) {
            _left_box_rectangle = aarectanglei{0, 0, theme().size(), context.height()};

            // The unknown_label is located to the right of the selection box icon.
            hilet option_rectangle = aarectanglei{
                _left_box_rectangle.right() + theme().margin<int>(),
                0,
                context.width() - _left_box_rectangle.width() - theme().margin<int>() * 2,
                context.height()};
            _off_label_shape = box_shape{_off_label_constraints, option_rectangle, theme().baseline_adjustment()};
            _current_label_shape = box_shape{_off_label_constraints, option_rectangle, theme().baseline_adjustment()};

        } else {
            _left_box_rectangle = aarectanglei{context.width() - theme().size(), 0, theme().size(), context.height()};

            // The unknown_label is located to the left of the selection box icon.
            hilet option_rectangle = aarectanglei{
                theme().margin<int>(),
                0,
                context.width() - _left_box_rectangle.width() - theme().margin<int>() * 2,
                context.height()};
            _off_label_shape = box_shape{_off_label_constraints, option_rectangle, theme().baseline_adjustment()};
            _current_label_shape = box_shape{_off_label_constraints, option_rectangle, theme().baseline_adjustment()};
        }

        _chevrons_glyph = find_glyph(elusive_icon::ChevronUp);
        hilet chevrons_glyph_bbox =
            narrow_cast<aarectanglei>(_chevrons_glyph.get_bounding_box() * narrow_cast<float>(theme().icon_size()));
        _chevrons_rectangle = align(_left_box_rectangle, chevrons_glyph_bbox, alignment::middle_center());
    }

    // The overlay itself will make sure the overlay fits the window, so we give the preferred size and position
    // from the point of view of the selection widget.
    // The overlay should start on the same left edge as the selection box and the same width.
    // The height of the overlay should be the maximum height, which will show all the options.
    hilet overlay_width =
        std::clamp(context.width() - theme().size(), _overlay_constraints.minimum.width(), _overlay_constraints.maximum.width());
    hilet overlay_height = _overlay_constraints.preferred.height();
    hilet overlay_x = os_settings::left_to_right() ? theme().size() : context.width() - theme().size() - overlay_width;
    hilet overlay_y = (context.height() - overlay_height) / 2;
    hilet overlay_rectangle_request = aarectanglei{overlay_x, overlay_y, overlay_width, overlay_height};
    hilet overlay_rectangle = make_overlay_rectangle(overlay_rectangle_request);
    _overlay_shape = box_shape{_overlay_constraints, overlay_rectangle, theme().baseline_adjustment()};
    _overlay_widget->set_layout(context.transform(_overlay_shape, 20.0f));

    _off_label_widget->set_layout(context.transform(_off_label_shape));
    _current_label_widget->set_layout(context.transform(_current_label_shape));
}

void selection_widget::draw(draw_context const& context) noexcept
{
    if (*mode > widget_mode::invisible) {
        if (overlaps(context, layout())) {
            draw_outline(context);
            draw_left_box(context);
            draw_chevrons(context);

            _off_label_widget->draw(context);
            _current_label_widget->draw(context);
        }

        // Overlay is outside of the overlap of the selection widget.
        _overlay_widget->draw(context);
    }
}

bool selection_widget::handle_event(gui_event const& event) noexcept
{
    switch (event.type()) {
    case gui_event_type::mouse_up:
        if (*mode >= widget_mode::partial and _has_options and layout().rectangle().contains(event.mouse().position)) {
            return handle_event(gui_event_type::gui_activate);
        }
        return true;

    case gui_event_type::gui_activate_next:
        // Handle gui_active_next so that the next widget will NOT get keyboard focus.
        // The previously selected item needs the get keyboard focus instead.
    case gui_event_type::gui_activate:
        if (*mode >= widget_mode::partial and _has_options and not _selecting) {
            start_selecting();
        } else {
            stop_selecting();
        }
        ++global_counter<"selection_widget:gui_activate:relayout">;
        process_event({gui_event_type::window_relayout});
        return true;

    case gui_event_type::gui_cancel:
        if (*mode >= widget_mode::partial and _has_options and _selecting) {
            stop_selecting();
        }
        ++global_counter<"selection_widget:gui_cancel:relayout">;
        process_event({gui_event_type::window_relayout});
        return true;

    default:;
    }

    return super::handle_event(event);
}

[[nodiscard]] hitbox selection_widget::hitbox_test(point2i position) const noexcept
{
    hi_axiom(loop::main().on_thread());

    if (*mode >= widget_mode::partial) {
        auto r = _overlay_widget->hitbox_test_from_parent(position);

        if (layout().contains(position)) {
            r = std::max(r, hitbox{this, _layout.elevation, _has_options ? hitbox_type::button : hitbox_type::_default});
        }

        return r;
    } else {
        return {};
    }
}

[[nodiscard]] bool selection_widget::accepts_keyboard_focus(keyboard_focus_group group) const noexcept
{
    hi_axiom(loop::main().on_thread());
    return *mode >= widget_mode::partial and to_bool(group & hi::keyboard_focus_group::normal) and _has_options;
}

[[nodiscard]] color selection_widget::focus_color() const noexcept
{
    hi_axiom(loop::main().on_thread());

    if (*mode >= widget_mode::partial and _has_options and _selecting) {
        return theme().color(semantic_color::accent);
    } else {
        return super::focus_color();
    }
}

[[nodiscard]] menu_button_widget const *selection_widget::get_first_menu_button() const noexcept
{
    hi_axiom(loop::main().on_thread());

    if (ssize(_menu_button_widgets) != 0) {
        return _menu_button_widgets.front();
    } else {
        return nullptr;
    }
}

[[nodiscard]] menu_button_widget const *selection_widget::get_selected_menu_button() const noexcept
{
    hi_axiom(loop::main().on_thread());

    for (hilet& button : _menu_button_widgets) {
        if (button->state() == button_state::on) {
            return button;
        }
    }
    return nullptr;
}

void selection_widget::start_selecting() noexcept
{
    hi_axiom(loop::main().on_thread());

    _selecting = true;
    _overlay_widget->mode = widget_mode::enabled;
    if (auto selected_menu_button = get_selected_menu_button()) {
        process_event(gui_event::window_set_keyboard_target(selected_menu_button, keyboard_focus_group::menu));

    } else if (auto first_menu_button = get_first_menu_button()) {
        process_event(gui_event::window_set_keyboard_target(first_menu_button, keyboard_focus_group::menu));
    }

    request_redraw();
}

void selection_widget::stop_selecting() noexcept
{
    hi_axiom(loop::main().on_thread());
    _selecting = false;
    _overlay_widget->mode = widget_mode::invisible;
    request_redraw();
}

/** Populate the scroll view with menu items corresponding to the options.
 */
void selection_widget::repopulate_options() noexcept
{
    hi_axiom(loop::main().on_thread());
    hi_assert_not_null(delegate);

    _column_widget->clear();
    _menu_button_widgets.clear();
    _menu_button_tokens.clear();

    auto [options, selected] = delegate->options_and_selected(*this);

    _has_options = size(options) > 0;

    // If any of the options has a an icon, all of the options should show the icon.
    auto show_icon = false;
    for (hilet& label : options) {
        show_icon |= to_bool(label.icon);
    }

    decltype(selected) index = 0;
    for (hilet& label : options) {
        auto menu_button = &_column_widget->make_widget<menu_button_widget>(selected, index, label, alignment, text_style);

        _menu_button_tokens.push_back(menu_button->pressed.subscribe(
            [this, index] {
                hi_assert_not_null(delegate);
                delegate->set_selected(*this, index);
                stop_selecting();
            },
            callback_flags::main));

        _menu_button_widgets.push_back(menu_button);

        ++index;
    }

    if (selected == -1) {
        _off_label_widget->mode = widget_mode::display;
        _current_label_widget->mode = widget_mode::invisible;

    } else {
        _off_label_widget->mode = widget_mode::invisible;
        _current_label_widget->label = options[selected];
        _current_label_widget->mode = widget_mode::display;
    }
}

void selection_widget::draw_outline(draw_context const& context) noexcept
{
    context.draw_box(
        layout(),
        layout().rectangle(),
        background_color(),
        focus_color(),
        theme().border_width(),
        border_side::inside,
        theme().rounding_radius());
}

void selection_widget::draw_left_box(draw_context const& context) noexcept
{
    hilet corner_radii = os_settings::left_to_right() ?
        hi::corner_radii(theme().rounding_radius<float>(), 0.0f, theme().rounding_radius<float>(), 0.0f) :
        hi::corner_radii(0.0f, theme().rounding_radius<float>(), 0.0f, theme().rounding_radius<float>());
    context.draw_box(layout(), translate_z(0.1f) * narrow_cast<aarectangle>(_left_box_rectangle), focus_color(), corner_radii);
}

void selection_widget::draw_chevrons(draw_context const& context) noexcept
{
    context.draw_glyph(
        layout(), translate_z(0.2f) * narrow_cast<aarectangle>(_chevrons_rectangle), _chevrons_glyph, label_color());
}

} // namespace hi::inline v1