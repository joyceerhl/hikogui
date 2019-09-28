// Copyright 2019 Pokitec
// All rights reserved.

#pragma once

#include "TTauri/Required/geometry.hpp"
#include "TTauri/Required/algorithm.hpp"
#include "TTauri/Required/numeric_cast.hpp"
#include <glm/glm.hpp>
#include <glm/gtx/vec_swizzle.hpp>
#include <fmt/format.h>
#include <boost/endian/conversion.hpp>
#include <string>
#include <algorithm>
#include <array>

namespace TTauri {


extern const std::array<int16_t,256> gamma_to_linear_i16_table;

gsl_suppress2(bounds.2,bounds.4)
inline int16_t gamma_to_linear_i16(uint8_t u) noexcept
{
    return gamma_to_linear_i16_table[u];
}

extern const std::array<uint8_t,4096> linear_to_gamma_u8_table;

gsl_suppress2(bounds.2,bounds.4)
inline uint8_t linear_to_gamma_u8(int16_t u) noexcept
{
    if (u >= 4096) {
        return 255;
    } else if (u < 0) {
        return 0;
    } else {
        return linear_to_gamma_u8_table[u];
    }
}

inline uint8_t linear_alpha_u8(int16_t u) noexcept
{
    if (u < 0) {
        return 0;
    } else {
        return (static_cast<uint32_t>(u) * 255 + 128) / 32767;
    }
}

inline int16_t linear_alpha_i16(uint8_t u) noexcept
{
    return static_cast<int16_t>((int32_t{u} * 32767 + 128) / 255);
}

/*! Wide Gammut linear sRGB with pre-mulitplied alpha.
 * This RGB space is compatible with sRGB but can represent colours outside of the
 * sRGB gammut. Becuase it is linear and has pre-multiplied alpha it is easy to use
 * for compositing.
 */
struct wsRGBA {
    glm::i16vec4 color;

    static constexpr int64_t I64_MAX_ALPHA = 32767;
    static constexpr int64_t I64_MAX_COLOR = 32767;
    static constexpr int64_t I64_MAX_SRGB = 4095;
    static constexpr float F32_MAX_ALPHA = I64_MAX_ALPHA;
    static constexpr float F32_ALPHA_MUL = 1.0f / F32_MAX_ALPHA;
    static constexpr float F32_MAX_SRGB = I64_MAX_SRGB;
    static constexpr float F32_SRGB_MUL = 1.0f / F32_MAX_SRGB;

    wsRGBA() noexcept : color({0, 0, 0, 0}) {}

    /*! Set the colour using the pixel value.
     * No conversion is done with the given value.
     */
    wsRGBA(glm::i16vec4 c) noexcept :
        color(c) {}

    /*! Set the colour with linear-sRGB values.
     * sRGB values are between 0.0 and 1.0, values outside of the sRGB color gammut should be between -0.5 - 7.5.
     * This constructor expect colour which has not been pre-multiplied with the alpha.
     */
    
    wsRGBA(glm::vec4 c) noexcept :
        color(static_cast<glm::i16vec4>(glm::vec4{
            glm::xyz(c) * c.a * F32_MAX_SRGB,
            c.a * F32_MAX_ALPHA })) {}

    /*! Set the colour with linear-sRGB values.
     * sRGB values are between 0.0 and 1.0, values outside of the sRGB color gammut should be between -0.5 - 7.5.
     * This constructor expect colour which has not been pre-multiplied with the alpha.
     */
    wsRGBA(double r, double g, double b, double a=1.0) noexcept :
        wsRGBA(glm::vec4{r, g, b, a}) {}

    /*! Set the colour with gamma corrected sRGB values.
     */
    
    wsRGBA(uint32_t c) noexcept {
        let colorWithoutPreMultiply = glm::i64vec4{
            gamma_to_linear_i16((c >> 24) & 0xff),
            gamma_to_linear_i16((c >> 16) & 0xff),
            gamma_to_linear_i16((c >> 8) & 0xff),
            linear_alpha_i16(c & 0xff)
        };

        color = static_cast<glm::i16vec4>(
            glm::i64vec4(
                (glm::xyz(colorWithoutPreMultiply) * colorWithoutPreMultiply.a) / I64_MAX_ALPHA,
                colorWithoutPreMultiply.a
            )
        );
    }

    int16_t const &r() const noexcept { return color.r; }
    int16_t const &g() const noexcept { return color.g; }
    int16_t const &b() const noexcept { return color.b; }
    int16_t const &a() const noexcept { return color.a; }

    int16_t &r() noexcept { return color.r; }
    int16_t &g() noexcept { return color.g; }
    int16_t &b() noexcept { return color.b; }
    int16_t &a() noexcept { return color.a; }

    int16_t operator[](size_t i) const {
        return color[static_cast<typename decltype(color)::length_type>(i)];
    }

    int16_t &operator[](size_t i) {
        return color[static_cast<typename decltype(color)::length_type>(i)];
    }

    bool isTransparent() const noexcept { return color.a <= 0; }
    bool isOpaque() const noexcept { return color.a == I64_MAX_ALPHA; }

    /*! Return a linear wsRGBA float vector with pre multiplied alpha.
     */
    
    glm::vec4 to_wsRGBApm_vec4() const noexcept {
        let floatColor = static_cast<glm::vec4>(color);
        return {
            glm::xyz(floatColor) * F32_SRGB_MUL,
            floatColor.a * F32_ALPHA_MUL
        };
    }

    
    glm::vec4 to_Linear_sRGBA_vec4() const noexcept {
        let floatColor = to_wsRGBApm_vec4();

        if (floatColor.a == 0) {
            return { 0.0, 0.0, 0.0, 0.0 };
        } else {
            let oneOverAlpha = 1.0f / floatColor.a;
            return {
                glm::xyz(floatColor) * oneOverAlpha,
                floatColor.a
            };
        }
    }

    /*! Return a 32 bit gamma corrected sRGBA colour with normal alpha.
    */
    
    uint32_t to_sRGBA_u32() const noexcept {
        let i64colorPM = static_cast<glm::i64vec4>(color);
        if (i64colorPM.a == 0) {
            return 0;
        }

        let i64color = glm::i64vec4{
            (glm::xyz(i64colorPM) * I64_MAX_ALPHA) / i64colorPM.a,
            i64colorPM.a
        };
        let i16color = static_cast<glm::i16vec4>(i64color);

        let red = linear_to_gamma_u8(i16color.r);
        let green = linear_to_gamma_u8(i16color.g);
        let blue = linear_to_gamma_u8(i16color.b);
        let alpha = linear_alpha_u8(i16color.a);
        return
            (static_cast<uint32_t>(red) << 24) |
            (static_cast<uint32_t>(green) << 16) |
            (static_cast<uint32_t>(blue) << 8) |
            static_cast<uint32_t>(alpha);
    }

    void desaturate(uint16_t brightness) noexcept {
        constexpr int64_t RY = static_cast<int64_t>(0.2126 * 32768.0);
        constexpr int64_t RG = static_cast<int64_t>(0.7152 * 32768.0);
        constexpr int64_t RB = static_cast<int64_t>(0.0722 * 32768.0);
        constexpr int64_t SCALE = static_cast<int64_t>(32768 * 32768);

        let _r = static_cast<int64_t>(r());
        let _g = static_cast<int64_t>(g());
        let _b = static_cast<int64_t>(b());

        int64_t y = ((
            RY * _r +
            RG * _g +
            RB * _b
        ) * brightness) / SCALE;
        r() = g() = b() = static_cast<int16_t>(std::clamp(
            y,
            static_cast<int64_t>(std::numeric_limits<int16_t>::min()),
            static_cast<int64_t>(std::numeric_limits<int16_t>::max())
        ));
    }

    void composit(wsRGBA over) noexcept {
        if (over.isTransparent()) {
            return;
        }
        if (over.isOpaque()) {
            color = over.color;
            return;
        }

        // 15 bit
        constexpr int64_t OVERV_MAX = I64_MAX_COLOR;
        let overV = static_cast<glm::i64vec4>(over.color);

        // 15 bit
        constexpr int64_t UNDERV_MAX = I64_MAX_COLOR;
        let underV = static_cast<glm::i64vec4>(color);

        // 15 bit
        constexpr int64_t ONE = OVERV_MAX;
        constexpr int64_t ONEMINUSOVERALPHA_MAX = OVERV_MAX;
        let oneMinusOverAlpha = ONE - overV.a;

        static_assert(OVERV_MAX * ONE == UNDERV_MAX * ONEMINUSOVERALPHA_MAX);

        // 15 bit + 15 bit == 15 bit + 15 bit
        constexpr int64_t RESULTV_MAX = UNDERV_MAX * ONEMINUSOVERALPHA_MAX;
        let resultV = (overV * ONE) + (underV * oneMinusOverAlpha);

        constexpr int64_t RESULTV_DIVIDER = RESULTV_MAX / I64_MAX_COLOR;
        static_assert(RESULTV_DIVIDER == 0x7fff);
        color = static_cast<glm::i16vec4>(resultV / RESULTV_DIVIDER);
    }

    void composit(wsRGBA over, uint8_t mask) noexcept {
        constexpr int64_t MASK_MAX = 255;

        if (mask == 0) {
            return;
        } else if (mask == MASK_MAX) {
            return composit(over);
        } else {
            // calculate 'over' by multiplying all components with the new alpha.
            // This means that the color stays pre-multiplied.
            constexpr int64_t NEWOVERV_MAX = I64_MAX_COLOR * MASK_MAX;
            let newOverV = static_cast<glm::i64vec4>(over.color) * static_cast<int64_t>(mask);

            constexpr int64_t NEWOVERV_DIVIDER = NEWOVERV_MAX / I64_MAX_COLOR;
            let newOver = wsRGBA{ static_cast<glm::i16vec4>(newOverV / NEWOVERV_DIVIDER) };
            return composit(newOver);
        }
    }

    void subpixelComposit(wsRGBA over, glm::u8vec3 mask) noexcept {
        constexpr int64_t MASK_MAX = 255;

        if (mask.r == mask.g && mask.r == mask.b) {
            return composit(over, mask.r);
        }

        // 8 bit
        constexpr int64_t MASKV_MAX = MASK_MAX;
        let maskV = glm::i64vec4{
            mask,
            (static_cast<int64_t>(mask.r) + static_cast<int64_t>(mask.g) + static_cast<int64_t>(mask.b)) / 3
        };

        // 15 bit
        constexpr int64_t UNDERV_MAX = I64_MAX_COLOR;
        let underV = static_cast<glm::i64vec4>(color);

        // 15 bit
        constexpr int64_t _OVERV_MAX = I64_MAX_COLOR;
        let _overV = static_cast<glm::i64vec4>(over.color);

        // The over color was already pre-multiplied with it's own alpha, so
        // it only needs to be pre multiplied with the mask.
        // 15 bit + 8 bit = 23 bit
        constexpr int64_t OVER_MAX = _OVERV_MAX * MASKV_MAX;
        let overV = _overV * maskV;

        // The alpha for each component is the subpixel-mask multiplied by the alpha of the original over.
        // 8 bit + 15 bit = 23 bit
        constexpr int64_t ALPHAV_MAX = MASKV_MAX * _OVERV_MAX;
        let alphaV = maskV * _overV.a;

        // 23 bit
        constexpr int64_t ONEMINUSOVERALPHAV_MAX = ALPHAV_MAX;
        constexpr glm::i64vec4 oneV = { ALPHAV_MAX, ALPHAV_MAX, ALPHAV_MAX, ALPHAV_MAX };
        let oneMinusOverAlphaV = oneV - alphaV;


        // 23 bit + 15 bit == 15bit + 23bit == 38bit
        constexpr int64_t ONE = 0x7fff;
        static_assert(OVER_MAX * ONE == UNDERV_MAX * ONEMINUSOVERALPHAV_MAX);
        constexpr int64_t RESULTV_MAX = OVER_MAX * ONE;
        let resultV = (overV * ONE) + (underV * oneMinusOverAlphaV);

        // 38bit - 15bit = 23bit.
        constexpr int64_t RESULTV_DIVIDER = RESULTV_MAX / I64_MAX_COLOR;
        static_assert(RESULTV_DIVIDER == 0x7fff * 0xff);
        color = static_cast<glm::i16vec4>(resultV / RESULTV_DIVIDER);
    }
};

inline bool operator==(wsRGBA const &lhs, wsRGBA const &rhs) noexcept
{
    return lhs.color == rhs.color;
}

inline bool operator<(wsRGBA const &lhs, wsRGBA const &rhs) noexcept
{
    if (lhs.color[0] != rhs.color[0]) {
        return lhs.color[0] < rhs.color[0];
    } else if (lhs.color[1] != rhs.color[1]) {
        return lhs.color[1] < rhs.color[1];
    } else if (lhs.color[2] != rhs.color[2]) {
        return lhs.color[2] < rhs.color[2];
    } else if (lhs.color[3] != rhs.color[3]) {
        return lhs.color[3] < rhs.color[3];
    } else {
        return false;
    }
}

inline bool operator!=(wsRGBA const &lhs, wsRGBA const &rhs) noexcept { return !(lhs == rhs); }
inline bool operator>(wsRGBA const &lhs, wsRGBA const &rhs) noexcept { return rhs < lhs; }
inline bool operator<=(wsRGBA const &lhs, wsRGBA const &rhs) noexcept { return !(lhs > rhs); }
inline bool operator>=(wsRGBA const &lhs, wsRGBA const &rhs) noexcept { return !(lhs < rhs); }

inline std::string to_string(wsRGBA const &x) noexcept
{
    let floatColor = x.to_wsRGBApm_vec4();
    if (
        floatColor.r >= 0.0 && floatColor.r <= 1.0 &&
        floatColor.g >= 0.0 && floatColor.g <= 1.0 &&
        floatColor.b >= 0.0 && floatColor.b <= 1.0
        ) {
        // This color is inside the sRGB gamut.
        return fmt::format("#{:08x}", x.to_sRGBA_u32());

    } else {
        return fmt::format("<{:.3f}, {:.3f}, {:.3f}, {:.3f}>", floatColor.r, floatColor.g, floatColor.b, floatColor.a);
    }
}

// http://www.brucelindbloom.com/index.html?Eqn_RGB_XYZ_Matrix.html
const glm::mat3x3 matrix_sRGB_to_XYZ = {
    0.4124564, 0.3575761, 0.1804375,
    0.2126729, 0.7151522, 0.0721750,
    0.0193339, 0.1191920, 0.9503041
};

const glm::mat3x3 matrix_XYZ_to_sRGB = {
    3.2404542, -1.5371385, -0.4985314,
    -0.9692660, 1.8760108, 0.0415560,
    0.0556434, -0.2040259,  1.0572252
};

}

namespace std {

template<>
class hash<TTauri::wsRGBA> {
public:
    size_t operator()(TTauri::wsRGBA const &v) const {
        return
            hash<int16_t>{}(v[0]) ^
            hash<int16_t>{}(v[1]) ^
            hash<int16_t>{}(v[2]) ^
            hash<int16_t>{}(v[3]);
    }
};

}
