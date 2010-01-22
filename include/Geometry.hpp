// Ares, a tactical space combat game.
// Copyright (C) 1997, 1999-2001, 2008 Nathan Lamont
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#ifndef ANTARES_GEOMETRY_HPP_
#define ANTARES_GEOMETRY_HPP_

#include <stdint.h>

namespace sfz { class BinaryReader; }

namespace antares {

struct Rect;

// A point (h, v) in two-dimensional space.
struct Point {
    int32_t h;
    int32_t v;

    // Creates a point at (0, 0).
    Point();

    // Creates a point at (x, y).
    // @param [in] x        The desired value of `h`.
    // @param [in] y        The desired value of `v`.
    Point(int x, int y);

    // Move the point to the nearest point within `rect`.
    // @param [in] rect     The rectangle to clamp to.
    void clamp_to(const Rect& rect);

    // Reads in the point via a BinaryReader.
    //
    // @param [in,out] bin  Used to read binary data.
    void read(sfz::BinaryReader* bin);
};

// A rectangle in two-dimensional space.
//
// Rectangles are represented as 4-tuples of (left, top, right, bottom).  The four corners of the
// rectangle are therefore located at (left, top), (right, top), (left, bottom), and (right,
// bottom).  The width is `right - left` and the height is `bottom - top`.
//
// Rect generally assumes the invariant that `right >= left && bottom >= top`.  Methods on Rect
// will return undefined results when this invariant does not hold, and other users of the class
// will probably have unpredictable results as well.
struct Rect {
    int32_t left;
    int32_t top;
    int32_t right;
    int32_t bottom;

    // Creates a zero-area rect at (0, 0).
    Rect();

    // Creates a rect with the four given parameters.
    // @param [in] left     The desired value of `left`.
    // @param [in] top      The desired value of `top`.
    // @param [in] right    The desired value of `right`.
    // @param [in] bottom   The desired value of `bottom`.
    Rect(int32_t left, int32_t top, int32_t right, int32_t bottom);

    // @returns             The width of the rectangle, which is `right - left`.
    int32_t width() const;

    // @returns             The height of the rectangle, which is `bottom - top`.
    int32_t height() const;

    // @returns             The area of the rectangle, which is `width() * height()`.
    int32_t area() const;

    // Returns true if this Rect contains `p`.
    //
    // A point is contained by a rectangle if the point's x-position is in the range [left, right)
    // and its y-position is in the range [top, bottom).
    //
    // @param [in] p        A Point to test.
    // @returns             true iff `p` is contained within this rect.
    bool contains(const Point& p) const;

    // Returns true if this Rect encloses `r`.
    //
    // @param [in] r        A Rect to test.
    // @returns             true iff all points in `r` are contained within this rect.
    bool encloses(const Rect& r) const;

    // Returns true if this Rect intersects `r`.
    //
    // @param [in] r        A Rect to test.
    // @returns             true iff any point in `r` is contained within this rect.
    bool intersects(const Rect& r) const;

    // Translates this Rect by `(x, y)`.
    //
    // @param [in] x        Added to `left` and `right`.
    // @param [in] y        Added to `top` and `bottom`.
    void offset(int32_t x, int32_t y);

    // Shrinks this Rect by `(x, y)`.
    //
    // Either of `x` and `y` may be negative, which would correspond to enlarging the rectangle in
    // the horizontal or vertical dimensions.  `x` must not be greater than `this->width() / 2`,
    // and `y` must not be greater than `this->height() / 2`.
    //
    // @param [in] x        Added to `left` and subtracted from `right`.
    // @param [in] y        Added to `top` and subtracted from `bottom`.
    void inset(int32_t x, int32_t y);

    // Translate this Rect so that it shares its center with `r`.
    //
    // @param [in] r        The Rect to center this rectangle within.
    void center_in(const Rect& r);

    // Intersect this Rect with `r`.
    //
    // After calling `clip_to()`, this Rect will contain only the points which were contained both
    // by `this` and `r` before the call.  If `this` and `r` do not contain any points in common,
    // it is possible that this Rect will no longer satisfy the rectangle invariant given in the
    // class documentation.
    //
    // @param [in] r        The Rect to clip this rectangle to.
    void clip_to(const Rect& r);

    // Enlarge this Rect to contain `r`.
    //
    // After calling `enlarge_to()`, this Rect will contain all points which were contained by
    // either `this` or `r` before the call.  It may also contain points which were contained by
    // neither, since the union of two rectangles is not necessarily a rectangle.
    //
    // @param [in] r        The Rect to enlarge this one around.
    void enlarge_to(const Rect& r);

    // Reads in the rect via a BinaryReader.
    //
    // @param [in,out] bin  used to read binary data.
    void read(sfz::BinaryReader* bin);
};

}  // namespace antares

#endif // ANTARES_GEOMETRY_HPP_
