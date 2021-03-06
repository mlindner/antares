// Copyright (C) 1997, 1999-2001, 2008 Nathan Lamont
// Copyright (C) 2016-2017 The Antares Authors
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

#ifndef ANTARES_GAME_INITIAL_HPP_
#define ANTARES_GAME_INITIAL_HPP_

#include "data/level.hpp"

namespace antares {

void                create_initial(Level::InitialObject* initial, uint32_t all_colors);
void                set_initial_destination(const Level::InitialObject* initial, bool preserve);
void                UnhideInitialObject(int32_t whichInitial);
Handle<SpaceObject> GetObjectFromInitialNumber(int32_t initialNumber);

}  // namespace antares

#endif  // ANTARES_GAME_INITIAL_HPP_
