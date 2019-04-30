// Copyright 2019 Pokitec
// All rights reserved.

#pragma once

#include "PipelineImage_ImageLocation.hpp"
#include "TTauri/geometry.hpp"
#include <vulkan/vulkan.hpp>

namespace TTauri::GUI::PipelineImage {

/*! A vertex defining a rectangle on a window.
* The vertex shader will convert window pixel-coordinates to normalized projection-coordinates.
*/
struct Vertex {
    //! The pixel-coordinates where the origin is located relative to the top-left corner of the window.
    glm::vec2 position;

    //! The position in pixels of the clipping rectangle relative to the top-left corner of the window, and extent in pixels.
    u16rect2 clippingRectangle;

    //! The x, y coord inside the texture-atlas, z is used as an index in the texture-atlas array
    u16vec3 atlasPosition;

    //! The depth for depth test.
    uint16_t depth;

    //! transparency of the image.
    uint8_t alpha;

    //! Align to 32 bits.
    uint8_t dummy[3];

    Vertex(const ImageLocation &location, glm::vec2 position, u16vec3 atlasPosition) :
        position(position),
        atlasPosition(atlasPosition),
        clippingRectangle({
            {static_cast<uint16_t>(location.clippingRectangle.offset.x), static_cast<uint16_t>(location.clippingRectangle.offset.y)},
            {static_cast<uint16_t>(location.clippingRectangle.extent.width()), static_cast<uint16_t>(location.clippingRectangle.extent.height())}
        }),
        depth(static_cast<uint16_t>(location.depth)),
        alpha(static_cast<uint8_t>(location.alpha * 255.0)) {}

    static vk::VertexInputBindingDescription inputBindingDescription()
    {
        return {
            0, sizeof(Vertex), vk::VertexInputRate::eVertex
        };
    }

    static std::vector<vk::VertexInputAttributeDescription> inputAttributeDescriptions()
    {
        return {
            { 0, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, position) },
            { 1, 0, vk::Format::eR16G16Uint, offsetof(Vertex, clippingRectangle.offset) },
            { 2, 0, vk::Format::eR16G16Uint, offsetof(Vertex, clippingRectangle.extent) },
            { 3, 0, vk::Format::eR16G16B16Uint, offsetof(Vertex, atlasPosition) },                
            { 4, 0, vk::Format::eR16Uint, offsetof(Vertex, depth) },
            { 5, 0, vk::Format::eR8Uint, offsetof(Vertex, alpha) },
        };
    }
};
}