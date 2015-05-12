// Copyright (C) 1997, 1999-2001, 2008 Nathan Lamont
// Copyright (C) 2008-2012 The Antares Authors
//
// This file is part of Antares, a tactical space combat game.
//
// Antares is free software: you can redistribute it and/or modify it
// under the terms of the Lesser GNU General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Antares is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with Antares.  If not, see http://www.gnu.org/licenses/

#ifndef ANTARES_VIDEO_DRIVER_HPP_
#define ANTARES_VIDEO_DRIVER_HPP_

#include <stdint.h>
#include <sfz/sfz.hpp>

#include "drawing/color.hpp"
#include "math/geometry.hpp"
#include "ui/event.hpp"

namespace antares {

class Card;
class KeyMap;
class PixMap;
class Sprite;

typedef std::unique_ptr<Sprite> Texture;

enum GameState {
    UNKNOWN,
    MAIN_SCREEN_INTERFACE,
    OPTIONS_INTERFACE,
    KEY_CONTROL_INTERFACE,
    SELECT_LEVEL_INTERFACE,
    MISSION_INTERFACE,
    PLAY_GAME,
    GAME_PAUSED,
    DONE_GAME,
};

class VideoDriver {
  public:
    VideoDriver();
    virtual ~VideoDriver();
    virtual bool button(int which) = 0;
    virtual Point get_mouse() = 0;
    virtual void get_keys(KeyMap* k) = 0;
    virtual InputMode input_mode() const = 0;

    virtual int ticks() const = 0;
    virtual int usecs() const = 0;
    virtual int64_t double_click_interval_usecs() const = 0;

    virtual Texture new_sprite(sfz::PrintItem name, const PixMap& content) = 0;
    virtual void dither_rect(const Rect& rect, const RgbColor& color) = 0;
    virtual void draw_point(const Point& at, const RgbColor& color) = 0;
    virtual void draw_line(const Point& from, const Point& to, const RgbColor& color) = 0;
    virtual void draw_triangle(const Rect& rect, const RgbColor& color) = 0;
    virtual void draw_diamond(const Rect& rect, const RgbColor& color) = 0;
    virtual void draw_plus(const Rect& rect, const RgbColor& color) = 0;

    static VideoDriver* driver();

  private:
    friend class Points;
    friend class Lines;
    friend class Rects;

    virtual void begin_points() { }
    virtual void end_points() { }
    virtual void batch_point(const Point& at, const RgbColor& color) { draw_point(at, color); }

    virtual void begin_lines() { }
    virtual void end_lines() { }
    virtual void batch_line(const Point& from, const Point& to, const RgbColor& color) {
        draw_line(from, to, color);
    }

    virtual void begin_rects() { }
    virtual void end_rects() { }
    virtual void batch_rect(const Rect& rect, const RgbColor& color) = 0;
};

class Sprite {
  public:
    virtual ~Sprite();
    virtual sfz::StringSlice name() const = 0;
    virtual void draw(const Rect& draw_rect) const = 0;
    virtual void draw_cropped(const Rect& draw_rect, Point origin, const RgbColor& tint=RgbColor::kWhite) const = 0;
    virtual void draw_shaded(const Rect& draw_rect, const RgbColor& tint) const = 0;
    virtual void draw_static(const Rect& draw_rect, const RgbColor& color, uint8_t frac) const = 0;
    virtual void draw_outlined(
            const Rect& draw_rect, const RgbColor& outline_color,
            const RgbColor& fill_color) const = 0;
    virtual const Size& size() const = 0;

    virtual void draw(int32_t x, int32_t y) const {
        draw(rect(x, y));
    }
    virtual void draw_shaded(int32_t x, int32_t y, const RgbColor& tint) const {
        draw_shaded(rect(x, y), tint);
    }
    virtual void draw_static(int32_t x, int32_t y, const RgbColor& color, uint8_t frac) const {
        draw_static(rect(x, y), color, frac);
    }
    virtual void draw_outlined(
            int32_t x, int32_t y, const RgbColor& outline_color,
            const RgbColor& fill_color) const {
        draw_outlined(rect(x, y), outline_color, fill_color);
    }

  private:
    friend class Quads;

    Rect rect(int32_t x, int32_t y) const {
        return Rect(x, y, x + size().width, y + size().height);
    }

    virtual void begin_quads() const { }
    virtual void end_quads() const { }
    virtual void draw_quad(const Rect& draw_rect, Point origin, const RgbColor& tint) const {
        draw_cropped(draw_rect, origin, tint);
    }
};

class Points {
  public:
    Points();
    ~Points();
    void draw(const Point& at, const RgbColor& color) const;
};

class Lines {
  public:
    Lines();
    ~Lines();
    void draw(const Point& from, const Point& to, const RgbColor& color) const;
};

class Rects {
  public:
    Rects();
    ~Rects();
    void fill(const Rect& rect, const RgbColor& color) const;
};

class Quads {
  public:
    Quads(const Sprite& sprite);
    ~Quads();
    void draw(const Rect& draw_rect, Point origin, const RgbColor& tint) const;
  private:
    const Sprite& _sprite;
};

}  // namespace antares

#endif  // ANTARES_VIDEO_DRIVER_HPP_
