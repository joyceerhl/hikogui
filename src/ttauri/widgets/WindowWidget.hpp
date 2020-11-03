// Copyright 2019 Pokitec
// All rights reserved.

#pragma once

#include "ContainerWidget.hpp"
#include "../l10n_label.hpp"

namespace tt {

class ToolbarWidget;
class GridLayoutWidget;

class WindowWidget final : public ContainerWidget {
public:
    WindowWidget(Window &window, GridLayoutDelegate *delegate, l10n_label title) noexcept;
    ~WindowWidget();

    void initialize() noexcept override;

    [[nodiscard]] bool update_constraints() noexcept override;
    [[nodiscard]] bool update_layout(hires_utc_clock::time_point display_time_point, bool need_layout) noexcept;
    [[nodiscard]] HitBox hitbox_test(vec window_position) const noexcept override;

    [[nodiscard]] vec backgroundColor() noexcept {
        tt_assume(GUISystem_mutex.recurse_lock_count());
        return theme->fillColor(_semantic_layer);
    }

    /** Defining on which edges the resize handle has priority over widget at a higher layer.
     */
    void set_resize_border_priority(bool left, bool right, bool bottom, bool top) noexcept
    {
        tt_assume(GUISystem_mutex.recurse_lock_count());
        left_resize_border_has_priority = left;
        right_resize_border_has_priority = right;
        bottom_resize_border_has_priority = bottom;
        top_resize_border_has_priority = top;
    }

    [[nodiscard]] std::shared_ptr<GridLayoutWidget> content() const noexcept
    {
        tt_assume(GUISystem_mutex.recurse_lock_count());
        tt_assume(_content);
        return _content;
    }

    [[nodiscard]] std::shared_ptr<ToolbarWidget> toolbar() const noexcept
    {
        tt_assume(GUISystem_mutex.recurse_lock_count());
        tt_assume(_toolbar);
        return _toolbar;
    }

private:
    l10n_label title;
    GridLayoutDelegate *_content_delegate;
    std::shared_ptr<GridLayoutWidget> _content;
    std::shared_ptr<ToolbarWidget> _toolbar;

    bool left_resize_border_has_priority = true;
    bool right_resize_border_has_priority = true;
    bool bottom_resize_border_has_priority = true;
    bool top_resize_border_has_priority = true;
};

} // namespace tt
