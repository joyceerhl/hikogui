// Copyright 2020 Pokitec
// All rights reserved.

#pragma once

#include "TTauri/Text/Grapheme.hpp"
#include "TTauri/Foundation/tagged_id.hpp"
#include <algorithm>
#include <utility>

namespace tt {

struct glyph_id_tag {};

using GlyphID = tagged_id<uint16_t, glyph_id_tag>;

};
