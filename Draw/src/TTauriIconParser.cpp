
#include "TTauri/Draw/TTauriIconParser.hpp"
#include "TTauri/Draw/Path.hpp"
#include "TTauri/Foundation/FileView.hpp"
#include "TTauri/Diagnostic/exceptions.hpp"
#include "TTauri/Foundation/span.hpp"
#include "TTauri/Required/wsRGBA.hpp"
#include <boost/endian/buffers.hpp>

using namespace boost::endian;

namespace TTauri::Draw {

struct little_fixed1_14_buf_t {
    little_int16_buf_t v;

    float value() const noexcept {
        return (v.value()) / 16384.0f;
    }
};

struct little_fixed1_13_buf_t {
    little_int16_buf_t v;

    float value() const noexcept {
        return (v.value() >> 1) / 8192.0f;
    }

    bool flag() const noexcept {
        return (v.value() & 1) > 0;
    }
};

struct little_point_buf_t {
    little_fixed1_13_buf_t x;
    little_fixed1_13_buf_t y;

    glm::vec2 coord() const noexcept {
        return { x.value(), y.value() };
    }

    BezierPoint::Type type() const noexcept {
        let type = (x.flag() ? 1 : 0) | (y.flag() ? 2 : 0);
        switch (type) {
        case 0b00: return BezierPoint::Type::Anchor;
        case 0b11: return BezierPoint::Type::QuadraticControl;
        case 0b01: return BezierPoint::Type::CubicControl1;
        case 0b10: return BezierPoint::Type::CubicControl2;
        default: no_default;
        }
    }

    BezierPoint value() const noexcept {
        return {coord(), type()};
    }
};

/*! wide gamut scRGB color.
 */
struct little_scRGB_buf_t {
    little_uint16_buf_t red;
    little_uint16_buf_t green;
    little_uint16_buf_t blue;
    little_uint16_buf_t alpha;

    wsRGBA value() const noexcept {
        return {glm::vec4{
            (static_cast<float>(red.value()) - 4096.0f) / 8192.0f,
            (static_cast<float>(green.value()) - 4096.0f) / 8192.0f,
            (static_cast<float>(blue.value()) - 4096.0f) / 8192.0f,
            static_cast<float>(alpha.value()) / 65535.0f
        }};
    }
};

struct header_buf_t {
    uint8_t magic[4];
    little_uint16_buf_t nr_paths;
};

struct path_buf_t {
    little_scRGB_buf_t fill_color;
    little_scRGB_buf_t stroke_color;
    little_fixed1_13_buf_t stroke_width;
    little_uint16_buf_t nr_contours;
};

struct contour_buf_t {
    little_uint16_buf_t nr_points;
};

struct Layer {
    Path path;
    wsRGBA fillColor;
    wsRGBA strokeColor;
    float strokeWidth;
    LineJoinStyle lineJoinStyle;
};

static std::vector<BezierPoint> parseContour(gsl::span<std::byte const> bytes, size_t &offset) noexcept
{
    let &header = at<contour_buf_t>(bytes, offset);
    offset += sizeof(contour_buf_t);

    let nr_points = to_signed(header.nr_points.value());

    auto contour = std::vector<BezierPoint>{};
    contour.reserve(nr_points);

    for (int i = 0; i < nr_points; i++) {
        let &point = at<little_point_buf_t>(bytes, offset);
        offset += sizeof(little_point_buf_t);

        contour.push_back(point.value());
    }

    return contour;
}

static Layer parsePath(gsl::span<std::byte const> bytes, size_t &offset) noexcept
{
    let &header = at<path_buf_t>(bytes, offset);
    offset += sizeof(path_buf_t);

    auto layer = Layer{};
    layer.fillColor = header.fill_color.value();
    layer.strokeColor = header.stroke_color.value();
    layer.strokeWidth = header.stroke_width.value();
    layer.lineJoinStyle = header.stroke_width.flag() ? LineJoinStyle::Bevel : LineJoinStyle::Miter;

    let nr_contours = to_signed(header.nr_contours.value());
    for (int i = 0; i < nr_contours; i++) {
        layer.path.addContour(parseContour(bytes, offset));
    }

    return layer;
}

Path parseTTauriIcon(gsl::span<std::byte const> bytes)
{
    size_t offset = 0;

    let &header = at<header_buf_t>(bytes, offset);
    offset += sizeof(header_buf_t);

    if (!(
        header.magic[0] == 'T' &&
        header.magic[1] == 'T' &&
        header.magic[2] == 'I' &&
        header.magic[3] == 'C'
    )) {
        TTAURI_THROW(parse_error("Expected 'TTIC' magic in header"));
    }

    let nr_paths = to_signed(header.nr_paths.value());

    auto drawing = Path{};
    for (int i = 0; i < nr_paths; i++) {
        let layer = parsePath(bytes, offset);

        if (!layer.fillColor.isTransparent()) {
            drawing.addPath(layer.path, layer.fillColor);
        }
        if (!layer.strokeColor.isTransparent()) {
            drawing.addStroke(layer.path, layer.strokeColor, layer.strokeWidth, layer.lineJoinStyle);
        }
    }

    drawing.advance = {1.0, 0.0};
    drawing.ascender = {0.0, 1.0};
    drawing.descender = {0.0, 0.0};
    drawing.capHeight = {0.0, 1.0};
    drawing.xHeight = {1.0, 0.0};

    return drawing;
}

}