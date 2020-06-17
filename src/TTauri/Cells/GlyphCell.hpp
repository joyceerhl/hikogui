// Copyright 2020 Pokitec
// All rights reserved.

#pragma once

#include "TTauri/Cells/ImageCell.hpp"
#include "TTauri/Text/FontGlyphIDs.hpp"
#include "TTauri/Foundation/aarect.hpp"

namespace tt {

class GlyphCell : public ImageCell {
    FontGlyphIDs glyph;
    aarect boundingBox;

public:
    GlyphCell(FontGlyphIDs glyph);

    GlyphCell(GlyphCell const &) noexcept = delete;
    GlyphCell(GlyphCell &&) noexcept = delete;
    GlyphCell &operator=(GlyphCell const &) noexcept = delete;
    GlyphCell &operator=(GlyphCell &&) noexcept = delete;

    void prepareForDrawing(Window &device) noexcept override;

    [[nodiscard]] bool draw(DrawContext const &drawContext, aarect rectangle, Alignment alignment) noexcept override;
};

}