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

#include "ui/screen.hpp"

#include <sfz/sfz.hpp>

#include "config/keys.hpp"
#include "data/resource.hpp"
#include "drawing/color.hpp"
#include "drawing/pix-map.hpp"
#include "game/time.hpp"
#include "sound/fx.hpp"
#include "ui/interface-handling.hpp"
#include "video/driver.hpp"

using sfz::Exception;
using sfz::Json;
using sfz::String;
using sfz::StringMap;
using sfz::StringSlice;
using sfz::format;
using sfz::read;
using sfz::string_to_json;
using std::unique_ptr;
using std::vector;

namespace utf8 = sfz::utf8;

namespace antares {

InterfaceScreen::InterfaceScreen(Id id, const Rect& bounds, bool full_screen)
        : _state(NORMAL),
          _bounds(bounds),
          _full_screen(full_screen),
          _hit_item(0) {
    Resource rsrc("interfaces", "json", id);
    String in(utf8::decode(rsrc.data()));
    Json json;
    if (!string_to_json(in, json)) {
        throw Exception("invalid interface JSON");
    }
    _items = interface_items(json);
    const int offset_x = (_bounds.width() / 2) - 320;
    const int offset_y = (_bounds.height() / 2) - 240;
    for (auto& item: _items) {
        item.bounds.offset(offset_x, offset_y);
    }
}

InterfaceScreen::~InterfaceScreen() { }

void InterfaceScreen::become_front() {
    this->adjust_interface();
    // half-second fade from black.
}

void InterfaceScreen::draw() const {
    Rect copy_area;
    if (_full_screen) {
        copy_area = _bounds;
    } else {
        next()->draw();
        GetAnyInterfaceItemGraphicBounds(_items[0], &copy_area);
        for (size_t i = 1; i < _items.size(); ++i) {
            Rect r;
            GetAnyInterfaceItemGraphicBounds(_items[i], &r);
            copy_area.enlarge_to(r);
        }
    }

    copy_area.offset(_bounds.left, _bounds.top);
    VideoDriver::driver()->fill_rect(copy_area, RgbColor::kBlack);

    for (vector<interfaceItemType>::const_iterator it = _items.begin(); it != _items.end(); ++it) {
        interfaceItemType copy = *it;
        copy.bounds.left += _bounds.left;
        copy.bounds.top += _bounds.top;
        copy.bounds.right += _bounds.left;
        copy.bounds.bottom += _bounds.top;
        draw_interface_item(copy);
    }
}

void InterfaceScreen::mouse_down(const MouseDownEvent& event) {
    Point where = event.where();
    where.h -= _bounds.left;
    where.v -= _bounds.top;
    if (event.button() != 0) {
        return;
    }
    for (size_t i = 0; i < _items.size(); ++i) {
        interfaceItemType* const item = &_items[i];
        Rect bounds;
        GetAnyInterfaceItemGraphicBounds(*item, &bounds);
        if (item->status() != kDimmed && bounds.contains(where)) {
            switch (item->kind) {
              case kPlainButton:
              case kCheckboxButton:
              case kRadioButton:
              case kTabBoxButton:
                _state = MOUSE_DOWN;
                item->set_status(kIH_Hilite);
                PlayVolumeSound(kComputerBeep1, kMediumLoudVolume, kShortPersistence,
                        kMustPlaySound);
                _hit_item = i;
                return;

              case kLabeledRect:
                return;

              case kListRect:
                throw Exception("kListRect not yet handled");

              default:
                break;
            }
        }
    }
    return;
}

void InterfaceScreen::mouse_up(const MouseUpEvent& event) {
    Point where = event.where();
    where.h -= _bounds.left;
    where.v -= _bounds.top;
    if (event.button() != 0) {
        return;
    }
    if (_state == MOUSE_DOWN) {
        // Save _hit_item and set it to 0 before calling handle_button(), as calling
        // handle_button() can result in the deletion of `this`.
        int hit_item = _hit_item;
        _hit_item = 0;

        _state = NORMAL;
        interfaceItemType* const item = &_items[hit_item];
        Rect bounds;
        GetAnyInterfaceItemGraphicBounds(*item, &bounds);
        item->set_status(kActive);
        if (bounds.contains(where)) {
            handle_button(hit_item);
        }
    }
    return;
}

void InterfaceScreen::mouse_move(const MouseMoveEvent& event) {
    // TODO(sfiera): highlight and un-highlight clicked button as dragged in and out.
    static_cast<void>(event);
}

void InterfaceScreen::key_down(const KeyDownEvent& event) {
    const int32_t key_code = event.key() + 1;
    for (size_t i = 0; i < _items.size(); ++i) {
        interfaceItemType* const item = &_items[i];
        if (item->status() != kDimmed && item->key() == key_code) {
            _state = KEY_DOWN;
            item->set_status(kIH_Hilite);
            PlayVolumeSound(kComputerBeep1, kMediumLoudVolume, kShortPersistence, kMustPlaySound);
            _hit_item = i;
            return;
        }
    }
}

void InterfaceScreen::key_up(const KeyUpEvent& event) {
    // TODO(sfiera): verify that the same key that was pressed was released.
    static_cast<void>(event);
    if (_state == KEY_DOWN) {
        // Save _hit_item and set it to 0 before calling handle_button(), as calling
        // handle_button() can result in the deletion of `this`.
        int hit_item = _hit_item;
        _hit_item = 0;

        _state = NORMAL;
        interfaceItemType* const item = &_items[hit_item];
        item->set_status(kActive);
        if (item->kind == kTabBoxButton) {
            item->item.radioButton.on = true;
        }
        handle_button(hit_item);
    }
}

void InterfaceScreen::adjust_interface() { }

void InterfaceScreen::truncate(size_t size) {
    if (size > _items.size()) {
        throw Exception("");
    }
    _items.resize(size);
}

void InterfaceScreen::extend(const std::vector<interfaceItemType>& items) {
    size_t size = _items.size();
    _items.insert(_items.end(), items.begin(), items.end());
    const int offset_x = (_bounds.width() / 2) - 320;
    const int offset_y = (_bounds.height() / 2) - 240;
    for (size_t i = size; i < _items.size(); ++i) {
        _items[i].bounds.offset(offset_x, offset_y);
    }
}

size_t InterfaceScreen::size() const {
    return _items.size();
}

const interfaceItemType& InterfaceScreen::item(int i) const {
    return _items[i];
}

interfaceItemType* InterfaceScreen::mutable_item(int i) {
    return &_items[i];
}

void InterfaceScreen::offset(int offset_x, int offset_y) {
    for (vector<interfaceItemType>::iterator it = _items.begin(); it != _items.end(); ++it) {
        it->bounds.offset(offset_x, offset_y);
    }
}

}  // namespace antares
