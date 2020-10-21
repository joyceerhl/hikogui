// Copyright 2020 Pokitec
// All rights reserved.

#pragma once

#include "Widget.hpp"
#include "../GUI/DrawContext.hpp"
#include "../text/format10.hpp"
#include "../observable.hpp"
#include <memory>
#include <string>
#include <array>
#include <optional>
#include <future>

namespace tt {

template<int ActiveValue>
class ToolbarTabButtonWidget final : public Widget {
public:
    observable<int> value;
    observable<std::u8string> label;

    template<typename V, typename... Args>
    ToolbarTabButtonWidget(Window &window, Widget *parent, V &&value, l10n const &fmt, Args const &... args) noexcept :
        Widget(window, parent), value(std::forward<V>(value)), label(format(fmt, args...))
    {
        _value_callback = scoped_callback(value, [this](auto...) {
            this->window.requestRedraw = true;
        });
        _label_callback = scoped_callback(label, [this](auto...) {
            request_reconstrain = true;
        });
    }

    template<typename V>
    ToolbarTabButtonWidget(Window &window, Widget *parent, V &&value) noexcept :
        ToolbarTabButtonWidget(window, parent, std::forward<V>(value), l10n{})
    {
    }

    ToolbarTabButtonWidget(Window &window, Widget *parent) noexcept :
        ToolbarTabButtonWidget(window, parent, observable<int>{}, l10n{})
    {
    }

    ~ToolbarTabButtonWidget() {}

    [[nodiscard]] bool update_constraints() noexcept override
    {
        tt_assume(mutex.is_locked_by_current_thread());

        if (Widget::update_constraints()) {
            label_cell = std::make_unique<TextCell>(*label, theme->labelStyle);

            ttlet minimumHeight = label_cell->preferredExtent().height();
            ttlet minimumWidth = label_cell->preferredExtent().width() + 2.0f * Theme::margin;

            p_preferred_size = {vec{minimumWidth, minimumHeight}, vec{minimumWidth, std::numeric_limits<float>::infinity()}};
            p_preferred_base_line = relative_base_line{VerticalAlignment::Middle, -Theme::margin};
            return true;
        } else {
            return false;
        }
    }

    [[nodiscard]] bool update_layout(hires_utc_clock::time_point display_time_point, bool need_layout) noexcept override
    {
        tt_assume(mutex.is_locked_by_current_thread());

        need_layout |= std::exchange(request_relayout, false);
        if (need_layout) {
            ttlet offset = Theme::margin + Theme::borderWidth;
            button_rectangle = aarect{
                rectangle().x(), rectangle().y() - offset, rectangle().width(), rectangle().height() + offset
            };
        }

        return Widget::update_layout(display_time_point, need_layout);
    }

    void draw(DrawContext context, hires_utc_clock::time_point display_time_point) noexcept override
    {
        tt_assume(mutex.is_locked_by_current_thread());

        drawButton(context);
        draw_label(context);
        drawFocusLine(context);
        Widget::draw(std::move(context), display_time_point);
    }

    bool handle_mouse_event(MouseEvent const &event) noexcept override
    {
        ttlet lock = std::scoped_lock(mutex);
        auto handled = Widget::handle_mouse_event(event);
        
        if (event.cause.leftButton) {
            handled = true;
            if (*enabled) {
                if (event.type == MouseEvent::Type::ButtonUp) {
                    ttlet position = from_window_transform * event.position;
                    if (button_rectangle.contains(position)) {
                        handle_command(command::gui_activate);
                    }
                }
            }
        }
        return handled;
    }

    bool handle_command(command command) noexcept override
    {
        ttlet lock = std::scoped_lock(mutex);
        auto handled = Widget::handle_command(command);

        if (*enabled) {
            if (command == command::gui_activate) {
                handled = true;
                if (compare_then_assign(value, ActiveValue)) {
                    window.requestRedraw = true;
                }
            }
        }
        return handled;
    }

    [[nodiscard]] HitBox hitbox_test(vec window_position) const noexcept override
    {
        ttlet lock = std::scoped_lock(mutex);

        if (p_window_clipping_rectangle.contains(window_position)) {
            return HitBox{this, p_draw_layer, *enabled ? HitBox::Type::Button : HitBox::Type::Default};
        } else {
            return HitBox{};
        }
    }

    [[nodiscard]] bool accepts_focus() const noexcept override
    {
        tt_assume(mutex.is_locked_by_current_thread());
        return *enabled;
    }

private:
    scoped_callback<decltype(value)> _value_callback;
    scoped_callback<decltype(label)> _label_callback;

    aarect button_rectangle;
    std::unique_ptr<TextCell> label_cell;

    void drawFocusLine(DrawContext const &context) noexcept
    {
        if (focus && window.active && *value == ActiveValue) {
            tt_assume(dynamic_cast<class ToolbarWidget *>(parent) != nullptr);

            // Draw the focus line over the full width of the window at the bottom
            // of the toolbar.
            auto parentContext = parent->make_draw_context(context);

            // Draw the line above every other direct child of the toolbar, and between
            // the selected-tab (0.6) and unselected-tabs (0.8).
            parentContext.transform = mat::T(0.0f, 0.0f, 1.7f) * parentContext.transform;

            parentContext.fillColor = theme->accentColor;
            parentContext.drawFilledQuad(aarect{
                parent->rectangle().x(), parent->rectangle().y(),
                parent->rectangle().width(), 1.0f
            });
        }
    }

    void drawButton(DrawContext context) noexcept
    {
        tt_assume(mutex.is_locked_by_current_thread());
        if (focus && window.active) {
            // The focus line will be placed at 0.7.
            context.transform = mat::T(0.0f, 0.0f, 0.8f) * context.transform;
        } else {
            context.transform = mat::T(0.0f, 0.0f, 0.6f) * context.transform;
        }

        // Override the clipping rectangle to match the toolbar.
        ttlet parent_lock = std::scoped_lock(parent->mutex);
        context.clippingRectangle = parent->window_rectangle();

        if (hover || *value == ActiveValue) {
            context.fillColor = theme->fillColor(p_semantic_layer - 2);
            context.color = context.fillColor;
        } else {
            context.fillColor = theme->fillColor(p_semantic_layer - 1);
            context.color = context.fillColor;
        }

        if (focus && window.active) {
            context.color = theme->accentColor;
        }

        context.cornerShapes = vec{0.0f, 0.0f, Theme::roundingRadius, Theme::roundingRadius};
        context.drawBoxIncludeBorder(button_rectangle);
    }

    void draw_label(DrawContext context) noexcept
    {
        tt_assume(mutex.is_locked_by_current_thread());

        context.transform = mat::T(0.0f, 0.0f, 0.9f) * context.transform;

        if (*enabled) {
            context.color = theme->labelStyle.color;
        }

        label_cell->draw(context, rectangle(), Alignment::MiddleCenter, base_line(), true);
    }
};

} // namespace tt
