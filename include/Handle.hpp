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

#ifndef ANTARES_HANDLE_HPP_
#define ANTARES_HANDLE_HPP_

#include <assert.h>
#include <stdint.h>
#include <vector>

#include "Resource.hpp"

#define DISALLOW_COPY_AND_ASSIGN(CLASS) \
  private: \
    CLASS(const CLASS&); \
    CLASS& operator=(const CLASS&);

template <typename T>
class TypedHandle {
  public:
    TypedHandle()
            : _data(NULL) { }

    TypedHandle clone() const {
        TypedHandle cloned;
        cloned.create(count());
        for (size_t i = 0; i < count(); ++i) {
            (*cloned)[i] = (**this)[i];
        }
        return cloned;
    }

    void create(int count) {
        _data = new Data(count);
    }

    void adopt(T* t, int count) {
        _data = new Data(t, count);
    }

    void destroy() {
        delete _data;
        _data = NULL;
    }

    void load_resource(uint32_t code, int id) {
        Resource rsrc(code, id);
        assert(rsrc.size() % sizeof(T) == 0);
        std::vector<T> loaded;
        const char* data = rsrc.data();
        size_t remainder = rsrc.size();
        while (remainder > 0) {
            loaded.push_back(T());
            size_t consumed = loaded.back().load_data(data, remainder);
            assert(consumed <= remainder);
            data += consumed;
            remainder -= consumed;
        }
        create(loaded.size());
        for (size_t i = 0; i < loaded.size(); ++i) {
            (**this)[i] = loaded[i];
        }
    }

    T* operator*() const {
        return _data->_ptr;
    }

    T** get() const {
        return &_data->_ptr;
    }

    size_t count() const {
        return _data->_count;
    }

    size_t size() const {
        return count() * sizeof(T);
    }

  private:
    class Data {
      public:
        Data(int count)
                : _ptr(new T[count]),
                  _count(count) { }

        Data(T* t, int count)
                : _ptr(t),
                  _count(count) { }

        ~Data() {
            delete[] _ptr;
        }

      private:
        friend class TypedHandle;

        T* _ptr;
        size_t _count;

        DISALLOW_COPY_AND_ASSIGN(Data);
    };
    Data* _data;
};

template <typename T>
TypedHandle<T> NewTypedHandle(int count) {
    TypedHandle<T> result;
    result.create(count);
    return result;
}

long Random();
// TypedHandle<>s can no longer be registered using mHandleLockAndRegister, but simply deleting the
// call thereto would prevent HHClearHandle() from being called.  HHClearHandle() makes a call to
// Random() for each byte contained in the handle, so this function ensures that the stream of
// random values is unaltered by the removal of the call to HHClearHandle().
template <typename T>
inline void TypedHandleClearHack(TypedHandle<T> handle) {
    for (size_t i = 0; i < handle.size(); ++i) {
        Random();
    }
}

#endif  // ANTARES_HANDLE_HPP_
