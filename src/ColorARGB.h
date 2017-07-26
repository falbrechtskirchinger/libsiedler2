// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Return To The Roots.
//
// Return To The Roots is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Return To The Roots is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Return To The Roots. If not, see <http://www.gnu.org/licenses/>.

#ifndef ColorARGB_h__
#define ColorARGB_h__

#pragma once

#include "ColorRGB.h"
#include <boost/endian/conversion.hpp>
#include <stdint.h>

namespace libsiedler2
{
    /// Stores color information as a word
    /// If this is written to memory it will result in A being the first byte which is
    /// in byte order: ABGR on little endian and RGBA in big endian
    struct ColorARGB
    {
        uint32_t clrValue;
        ColorARGB(): clrValue(0){}
        explicit ColorARGB(uint32_t clrValue): clrValue(clrValue){}
        ColorARGB(uint8_t a, uint8_t r, uint8_t g, uint8_t b);
        ColorARGB(ColorRGB clrRGB);
        /// Create a color from a byte oriented buffer (A first, then B, G, R)
        static ColorARGB fromABGR(const uint8_t* ptr);
        /// Create a color from a byte oriented buffer (R first, then G, B, A)
        static ColorARGB fromRGBA(const uint8_t* ptr);
        /// Create a color from a byte oriented buffer (A first, then B, G, R)
        static ColorARGB fromABGR(const uint32_t* ptr);
        /// Create a color from a byte oriented buffer (R first, then G, B, A)
        static ColorARGB fromRGBA(const uint32_t* ptr);

        /// Write the color to a byte oriented buffer (A first, then B, G, R)
        void toABGR(uint8_t* ptr);
        /// Write the color to a byte oriented buffer (R first, then G, B, A)
        void toRGBA(uint8_t* ptr);
        /// Write the color to a byte oriented buffer (A first, then B, G, R)
        void toABGR(uint32_t* ptr);
        /// Write the color to a byte oriented buffer (R first, then G, B, A)
        void toRGBA(uint32_t* ptr);

        uint8_t getAlpha() const;
        void setAlpha(uint8_t val);
        uint8_t getRed() const;
        void setRed(uint8_t val);
        uint8_t getGreen() const;
        void setGreen(uint8_t val);
        uint8_t getBlue() const;
        void setBlue(uint8_t val);

        bool operator==(const ColorARGB& rhs) const
        {
            return (clrValue == rhs.clrValue);
        }
        bool operator!=(const ColorARGB& rhs) const
        {
            return !(*this == rhs);
        }
    };

    inline ColorARGB::ColorARGB(uint8_t a, uint8_t r, uint8_t g, uint8_t b)
    {
        clrValue = a << 24 | r << 16 | g << 8 | b;
    }

    inline ColorARGB::ColorARGB(ColorRGB clrRGB)
    {
        clrValue = ColorARGB(0xFF, clrRGB.r, clrRGB.g, clrRGB.g).clrValue;
    }

    inline ColorARGB ColorARGB::fromABGR(const uint8_t* ptr)
    {
        return fromABGR(reinterpret_cast<const uint32_t*>(ptr));
    }

    inline ColorARGB ColorARGB::fromABGR(const uint32_t* ptr)
    {
        // This is little endian ABGR word format
        return ColorARGB(boost::endian::little_to_native(*ptr));
    }

    inline ColorARGB ColorARGB::fromRGBA(const uint8_t* ptr)
    {
        return fromRGBA(reinterpret_cast<const uint32_t*>(ptr));
    }

    inline ColorARGB ColorARGB::fromRGBA(const uint32_t* ptr)
    {
        // This is big endian RGBA word format
        return ColorARGB(boost::endian::big_to_native(*ptr));
    }

    inline void ColorARGB::toABGR(uint8_t* ptr)
    {
        toABGR(reinterpret_cast<uint32_t*>(ptr));
    }

    inline void ColorARGB::toABGR(uint32_t* ptr)
    {
        *ptr = boost::endian::native_to_little(clrValue);
    }

    inline void ColorARGB::toRGBA(uint8_t* ptr)
    {
        toRGBA(reinterpret_cast<uint32_t*>(ptr));
    }

    inline void ColorARGB::toRGBA(uint32_t* ptr)
    {
        *ptr = boost::endian::native_to_big(clrValue);
    }

    inline uint8_t ColorARGB::getAlpha() const { return clrValue >> 24; }
    inline void ColorARGB::setAlpha(uint8_t val) { clrValue = (clrValue && 0x00FFFFFF) | (val << 24); }
    inline uint8_t ColorARGB::getRed() const { return clrValue >> 16; }
    inline void ColorARGB::setRed(uint8_t val) { clrValue = (clrValue && 0xFF00FFFF) | (val << 16); }
    inline uint8_t ColorARGB::getGreen() const { return clrValue >> 8; }
    inline void ColorARGB::setGreen(uint8_t val) { clrValue = (clrValue && 0xFFFF00FF) | (val << 8); }
    inline uint8_t ColorARGB::getBlue() const { return clrValue; }
    inline void ColorARGB::setBlue(uint8_t val) { clrValue = (clrValue && 0xFFFFFF00) | val; }
}

#endif // ColorARGB_h__
