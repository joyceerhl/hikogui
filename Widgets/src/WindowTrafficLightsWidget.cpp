// Copyright 2019 Pokitec
// All rights reserved.

#include "TTauri/Widgets/WindowTrafficLightsWidget.hpp"
#include "TTauri/GUI/utils.hpp"
#include "TTauri/Foundation/utils.hpp"
#include <cmath>
#include <typeinfo>

namespace TTauri::GUI::Widgets {

WindowTrafficLightsWidget::WindowTrafficLightsWidget(Window &window, Widget *parent, Path applicationIcon) noexcept :
    Widget(window, parent), applicationIcon(std::move(applicationIcon))
{
    window.addConstraint(box.height == HEIGHT);
    window.addConstraint(box.width == WIDTH);
}

int WindowTrafficLightsWidget::state() const noexcept {
    int r = 0;
    r |= window.active ? 1 : 0;
    if constexpr (operatingSystem == OperatingSystem::MacOS) {
        r |= hover ? 2 : 0;
        r |= pressedRed ? 4 : 0;
        r |= pressedYellow ? 8 : 0;
        r |= pressedGreen ? 16 : 0;
        r |= (window.size == Window::Size::Maximized) ? 32 : 0;
    }
    return r;
}

void WindowTrafficLightsWidget::draw(DrawContext const &drawContext, cpu_utc_clock::time_point displayTimePoint) noexcept
{
    auto drawingBackingImage = backingImage.loadOrDraw(
        window,
        box.currentExtent(),
        [&](auto image) {
            return drawImage(image);
        },
        "WindowTrafficLightsWidget", state()
    );

    if (drawingBackingImage) {
        ++renderTrigger;
    }

    if (backingImage.image) {
        let currentScale = (box.currentExtent() / vec{backingImage.image->extent}).xy11();

        auto context = drawContext;
        context.transform = context.transform * mat::S(currentScale);
        context.drawImage(*(backingImage.image));
    }

    Widget::draw(drawContext, displayTimePoint);
}

void WindowTrafficLightsWidget::drawTrianglesOutward(Path &path, vec position, float radius) noexcept
{
    let L = radius * 0.5;
    let W = radius * 0.3;

    path.moveTo({position.x() - L, position.y() - L});
    path.lineTo({position.x() + W, position.y() - L});
    path.lineTo({position.x() - L, position.y() + W});
    path.closeContour();

    path.moveTo({position.x() + L, position.y() + L});
    path.lineTo({position.x() - W, position.y() + L});
    path.lineTo({position.x() + L, position.y() - W});
    path.closeContour();
}

void WindowTrafficLightsWidget::drawTrianglesInward(Path &path, vec position, float radius) noexcept
{
    let L = radius * 0.8;

    path.moveTo({position.x(), position.y()});
    path.lineTo({position.x() - L, position.y()});
    path.lineTo({position.x(), position.y() - L});
    path.closeContour();

    path.moveTo({position.x(), position.y()});
    path.lineTo({position.x() + L, position.y()});
    path.lineTo({position.x(), position.y() + L});
    path.closeContour();
}

void WindowTrafficLightsWidget::drawCross(Path &path, vec position, float radius) noexcept
{
    let W = sqrt(0.5);
    let L = radius * 0.5;
    
    // Left bottom line.
    path.moveTo({position.x() - W, position.y()});
    path.lineTo({position.x() - L, position.y() - L + W});
    path.lineTo({position.x() - L + W, position.y() - L});
    path.lineTo({position.x(), position.y() - W});

    // Right bottom line.
    path.lineTo({position.x() + L - W, position.y() - L});
    path.lineTo({position.x() + L, position.y() - L + W});
    path.lineTo({position.x() + W, position.y()});

    // Right top line.
    path.lineTo({position.x() + L, position.y() + L - W});
    path.lineTo({position.x() + L - W, position.y() + L});
    path.lineTo({position.x(), position.y() + W});

    // Left top line.
    path.lineTo({position.x() - L + W, position.y() + L});
    path.lineTo({position.x() - L, position.y() + L - W});

    path.closeContour();
}

PixelMap<R16G16B16A16SFloat> WindowTrafficLightsWidget::drawApplicationIconImage(PipelineImage::Image &image) noexcept
{
    auto linearMap = PixelMap<R16G16B16A16SFloat>{image.extent};
    fill(linearMap);

    let iconPath = applicationIcon.centerScale(vec{image.extent}, 3.0);
    composit(linearMap, iconPath);

    if (!window.active) {
        desaturate(linearMap, 0.5f);
    }
    return linearMap;
}

PixelMap<R16G16B16A16SFloat> WindowTrafficLightsWidget::drawTrafficLightsImage(PipelineImage::Image &image) noexcept
{
    let height = box.height.value();

    let redCenter = vec{
        MARGIN + RADIUS,
        height / 2.0
    };
    let yellowCenter = vec{
        MARGIN + DIAMETER + SPACING + RADIUS,
        height / 2.0
    };
    let greenCenter = vec{
        MARGIN + DIAMETER + SPACING + DIAMETER + SPACING + RADIUS,
        height / 2.0
    };

    auto drawing = Path();
    drawing.addCircle(redCenter, RADIUS);
    
    if (!window.active && !hover) {
        drawing.closeLayer(vec{0.246, 0.246, 0.246, 1.0});
    } else if (pressedRed) {
        drawing.closeLayer(vec{1.0, 0.242, 0.212, 1.0});
    } else {
        drawing.closeLayer(vec{1.0, 0.1, 0.082, 1.0});
    }

    drawing.addCircle(yellowCenter, RADIUS);
    if (!window.active && !hover) {
        drawing.closeLayer(vec{0.246, 0.246, 0.246, 1.0});
    } else if (pressedYellow) {
        drawing.closeLayer(vec{1.0, 0.847, 0.093, 1.0});
    } else {
        drawing.closeLayer(vec{0.784, 0.521, 0.021, 1.0});
    }

    drawing.addCircle(greenCenter, RADIUS);
    if (!window.active && !hover) {
        drawing.closeLayer(vec{0.246, 0.246, 0.246, 1.0});
    } else if (pressedGreen) {
        drawing.closeLayer(vec{0.223, 0.863, 0.1, 1.0});
    } else {
        drawing.closeLayer(vec{0.082, 0.533, 0.024, 1.0});
    }

    if (hover) {
        drawCross(drawing, redCenter, RADIUS);
        drawing.closeLayer(vec{0.319, 0.0, 0.0, 1.0});

        drawing.addRectangle({yellowCenter.x() - RADIUS * 0.5 - 0.5, yellowCenter.y() - 0.5, RADIUS * 1.0 + 1.0, 1.0});
        drawing.closeLayer(vec{0.212, 0.1, 0.0, 1.0});

        if (window.size == Window::Size::Maximized) {
            drawTrianglesInward(drawing, greenCenter, RADIUS);
        } else {
            drawTrianglesOutward(drawing, greenCenter, RADIUS);
        }
        drawing.closeLayer(vec{0.0, 0.133, 0.0, 1.0});
    }

    auto linearMap = PixelMap<R16G16B16A16SFloat>{image.extent};
    fill(linearMap);
    composit(linearMap, drawing);
    return linearMap;
}

PipelineImage::Backing::ImagePixelMap WindowTrafficLightsWidget::drawImage(std::shared_ptr<GUI::PipelineImage::Image> image) noexcept
{
    if constexpr (operatingSystem == OperatingSystem::Windows) {
        return { std::move(image), drawApplicationIconImage(*image) };
    } else if constexpr (operatingSystem == OperatingSystem::MacOS) {
        return { std::move(image), drawTrafficLightsImage(*image) };
    } else {
        no_default;
    }
}

std::tuple<rect, rect, rect, rect> WindowTrafficLightsWidget::getButtonRectangles() const noexcept
{
    let left = box.left.value();
    let bottom = box.bottom.value();
    let height = box.height.value();

    let sysmenuButtonBox = rect{
        {left, bottom},
        {height, height}
    };

    let redButtonBox = rect{
        {left + MARGIN, bottom + MARGIN},
        {DIAMETER, DIAMETER}
    };

    let yellowButtonBox = rect{
        {left + MARGIN + DIAMETER + SPACING, bottom + MARGIN},
        {DIAMETER, DIAMETER}
    };

    let greenButtonBox = rect{
        {left + MARGIN + DIAMETER * 2.0 + SPACING * 2.0, bottom + MARGIN},
        {DIAMETER, DIAMETER}
    };

    return {redButtonBox, yellowButtonBox, greenButtonBox, sysmenuButtonBox};    
}

void WindowTrafficLightsWidget::handleMouseEvent(MouseEvent const &event) noexcept
{
    Widget::handleMouseEvent(event);

    if constexpr (operatingSystem == OperatingSystem::Windows) {
        return;

    } else if constexpr (operatingSystem == OperatingSystem::MacOS) {
        let [redButtonRect, yellowButtonRect, greenButtonRect, sysmenuButtonBox] = getButtonRectangles();

        if (event.type == MouseEvent::Type::ButtonUp && event.cause.leftButton) {
            if (pressedRed) {
                window.closeWindow();
            } else if (pressedYellow) {
                window.minimizeWindow();
            } else if (pressedGreen) {
                switch (window.size) {
                case Window::Size::Normal:
                    window.maximizeWindow();
                    break;
                case Window::Size::Maximized:
                    window.normalizeWindow();
                    break;
                default:
                    no_default;
                }
            }
        }

        // Only change the pressed state after checking for Button Up, the
        // button up will check which button was pressed from button down.
        auto stateHasChanged = false;
        stateHasChanged |= assign_and_compare(pressedRed, event.down.leftButton && redButtonRect.contains(event.position));
        stateHasChanged |= assign_and_compare(pressedYellow, event.down.leftButton && yellowButtonRect.contains(event.position));
        stateHasChanged |= assign_and_compare(pressedGreen, event.down.leftButton && greenButtonRect.contains(event.position));
        if (stateHasChanged) {
            ++renderTrigger;
        }

    } else {
        no_default;
    }
}

HitBox WindowTrafficLightsWidget::hitBoxTest(vec position) noexcept
{
    let [redButtonRect, yellowButtonRect, greenButtonRect, sysmenuButtonBox] = getButtonRectangles();

    auto r = HitBox{};

    if constexpr (operatingSystem == OperatingSystem::Windows) {
        if (sysmenuButtonBox.contains(position)) {
            r = HitBox{this, elevation, HitBox::Type::ApplicationIcon};
        }

    } else if constexpr (operatingSystem == OperatingSystem::MacOS) {
        if (redButtonRect.contains(position) ||
            yellowButtonRect.contains(position) ||
            greenButtonRect.contains(position)
        ) {
            r = HitBox{this, elevation, HitBox::Type::Button};
        }

    } else {
        no_default;
    }

    return r;
}

}