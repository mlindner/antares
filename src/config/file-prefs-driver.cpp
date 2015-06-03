// Copyright (C) 1997, 1999-2001, 2008 Nathan Lamont
// Copyright (C) 2015 The Antares Authors
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

#include "config/file-prefs-driver.hpp"

#include <fcntl.h>
#include <sfz/sfz.hpp>

#include "config/dirs.hpp"
#include "config/keys.hpp"

using sfz::Exception;
using sfz::Json;
using sfz::MappedFile;
using sfz::ScopedFd;
using sfz::String;
using sfz::StringMap;
using sfz::StringSlice;
using sfz::format;
using sfz::range;
using std::vector;

namespace utf8 = sfz::utf8;

namespace antares {

static const char* kKeyNames[KEY_COUNT] = {
    "ship accel",
    "ship decel",
    "ship ccw",
    "ship cw",
    "fire 1",
    "fire 2",
    "fire s",
    "warp",

    "select friendly",
    "select hostile",
    "select base",
    "target",
    "order",
    "zoom in",
    "zoom out",

    "comp up",
    "comp down",
    "comp accept",
    "comp cancel",

    "transfer",
    "zoom 1:1",
    "zoom 1:2",
    "zoom 1:4",
    "zoom 1:16",
    "zoom hostile",
    "zoom object",
    "zoom all",
    "next message",
    "help",
    "volume down",
    "volume up",
    "game music",
    "net settings",
    "fast motion",

    "hotkey 1",
    "hotkey 2",
    "hotkey 3",
    "hotkey 4",
    "hotkey 5",
    "hotkey 6",
    "hotkey 7",
    "hotkey 8",
    "hotkey 9",
    "hotkey 10",
};

FilePrefsDriver::FilePrefsDriver() {
}

bool get(Json json, bool& v) {
    if (json.is_boolean()) {
        v = json.boolean();
        return true;
    }
    return false;
}

bool get(Json json, int& v) {
    if (json.is_number()) {
        v = json.number();
        return true;
    }
    return false;
}

template <typename ValueType, typename ValueKey, typename PrefsMethod, typename... Args>
static void set_from(
    const Json& json, const char* section_key, ValueKey value_key,
    Preferences& prefs, PrefsMethod pmeth, Args&& ...args) {
    auto section = json.get(section_key);
    auto value = section.get(value_key);
    ValueType typed;
    if (get(value, typed)) {
        (prefs.*pmeth)(args..., typed);
    }
}

void FilePrefsDriver::load(Preferences* p) {
    try {
        String path(format("{0}/config.json", dirs().root));
        MappedFile file(path);
        Json json;
        String data(utf8::decode(file.data()));
        if (!string_to_json(data, json)) {
            return;
        }

        set_from<bool>(json, "video", "fullscreen", *p, &Preferences::set_fullscreen);

        set_from<int>(json, "sound", "volume", *p, &Preferences::set_volume);
        set_from<bool>(json, "sound", "speech", *p, &Preferences::set_speech_on);
        set_from<bool>(json, "sound", "idle music", *p, &Preferences::set_play_idle_music);
        set_from<bool>(json, "sound", "game music", *p, &Preferences::set_play_music_in_game);

        for (auto i: range<size_t>(KEY_COUNT)) {
            set_from<int>(json, "keys", kKeyNames[i], *p, &Preferences::set_key, i);
        }
    } catch (Exception& e) {
        // pass
    }
}

void FilePrefsDriver::save(const Preferences& p) {
    StringMap<Json> video;
    video["fullscreen"]  = Json::boolean(p.fullscreen());
    video["width"]       = Json::number(p.screen_size().width);
    video["height"]      = Json::number(p.screen_size().height);

    StringMap<Json> sound;
    sound["volume"]      = Json::number(p.volume());
    sound["speech"]      = Json::boolean(p.speech_on());
    sound["idle music"]  = Json::boolean(p.play_idle_music());
    sound["game music"]  = Json::boolean(p.play_music_in_game());

    StringMap<Json> keys;
    for (auto i: range<size_t>(KEY_COUNT)) {
        keys[kKeyNames[i]] = Json::number(p.key(i));
    }

    StringMap<Json> all;
    all["video"]  = Json::object(video);
    all["sound"]  = Json::object(sound);
    all["keys"]   = Json::object(keys);

    String path(format("{0}/config.json", dirs().root));
    ScopedFd fd(open(path, O_CREAT | O_TRUNC | O_WRONLY, 0644));
    String pretty(pretty_print(Json::object(all)));
    write(fd, utf8::encode(pretty));
}

}  // namespace antares
