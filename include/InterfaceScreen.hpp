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

#ifndef ANTARES_INTERFACE_SCREEN_HPP_
#define ANTARES_INTERFACE_SCREEN_HPP_

#include <vector>
#include "Card.hpp"
#include "PlayerInterfaceItems.hpp"
#include "SmartPtr.hpp"

namespace antares {

class PixMap;

class InterfaceScreen : public Card {
  public:
    InterfaceScreen(int id);
    ~InterfaceScreen();

    virtual void become_front();

    virtual bool mouse_down(int button, const Point& where);
    virtual bool mouse_up(int button, const Point& where);
    virtual bool mouse_moved(int button, const Point& where);
    virtual bool key_down(int key);
    virtual bool key_up(int key);

  protected:
    double last_event() const;
    virtual void adjust_interface();
    virtual void handle_button(int button) = 0;
    virtual void draw() const;

    const interfaceItemType& item(int index) const;
    interfaceItemType* mutable_item(int index);

  private:

    enum State {
        NORMAL,
        MOUSE_DOWN,
        KEY_DOWN,
    };
    State _state;

    int _id;
    double _last_event;
    std::vector<interfaceItemType> _items;
    int _hit_item;

    DISALLOW_COPY_AND_ASSIGN(InterfaceScreen);
};

}  // namespace antares

#endif  // ANTARES_INTERFACE_SCREEN_HPP_
