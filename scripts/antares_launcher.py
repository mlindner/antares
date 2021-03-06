#!/usr/bin/env python
# Copyright (C) 2017 The Antares Authors
# This file is part of Antares, a tactical space combat game.
# Antares is free software, distributed under the LGPL+. See COPYING.

from __future__ import division, print_function, unicode_literals

import gi
import os
import subprocess
import sys

gi.require_version("Gtk", "3.0")
from gi.repository import Gtk


PREFIX = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
if os.path.exists(os.path.join(PREFIX, "games", "antares-glfw")):
    BIN_PREFIX = os.path.join(PREFIX, "games")
    DATADIR = ""
    ANTARES_ICON = os.path.join(PREFIX, "share", "icons", "hicolor", "128x128", "apps",
                                "antares.png")
    APP_DATA = ""
    SCENARIOS = os.path.join(PREFIX, "share", "games", "antares", "scenarios")
    FACTORY_SCENARIO = ""
else:
    BIN_PREFIX = os.path.join(PREFIX, "out", "cur")
    DATADIR = os.path.join(PREFIX, "resources")
    ANTARES_ICON = os.path.join(DATADIR, "antares.iconset", "icon_128x128.png")
    APP_DATA = os.path.join(PREFIX, "data")
    SCENARIOS = os.path.join(APP_DATA, "scenarios")
    FACTORY_SCENARIO = os.path.join(SCENARIOS, "com.biggerplanet.ares")

ANTARES_BIN = os.path.join(BIN_PREFIX, "antares-glfw")
INSTALL_DATA_BIN = os.path.join(BIN_PREFIX, "antares-install-data")
LS_SCENARIOS_BIN = os.path.join(BIN_PREFIX, "antares-ls-scenarios")


def main():
    reinstall_or_check_scenario()
    scenarios = ls_scenarios()
    win = LauncherWindow(scenarios)
    win.show_all()
    Gtk.main()


class LauncherWindow(Gtk.Dialog):

    def __init__(self, scenarios):
        Gtk.Dialog.__init__(
                self, "Antares", None, 0,
                ("Quit", Gtk.ResponseType.CANCEL, "Start", Gtk.ResponseType.OK),
                window_position=Gtk.WindowPosition.CENTER)
        self.set_default_response(Gtk.ResponseType.OK)
        self.connect("response", self.on_response)

        plugin = Gtk.Grid(row_homogeneous=True, hexpand=True, border_width=10)

        self.scenarios = scenarios

        plugin.attach(Gtk.Label("Scenario", xalign=1), 0, 1, 1, 1)
        self.download = Gtk.LinkButton("", label="", hexpand=True, xalign=0, margin_left=10)
        plugin.attach(self.download, 1, 1, 1, 1)

        plugin.attach(Gtk.Label("Author", xalign=1), 0, 2, 1, 1)
        self.author = Gtk.LinkButton("", label="", hexpand=True, xalign=0, margin_left=10)
        plugin.attach(self.author, 1, 2, 1, 1)

        plugin.attach(Gtk.Label("Version", xalign=1), 0, 3, 1, 1)
        self.version = Gtk.Label(
                "", hexpand=True, justify=Gtk.Justification.LEFT, xalign=0, margin_left=10)
        plugin.attach(self.version, 1, 3, 1, 1)

        scenario_combo = Gtk.ComboBoxText()
        scenario_combo.set_entry_text_column(0)
        scenario_combo.connect("changed", self.on_change_scenario)
        for scenario in scenarios:
            scenario_combo.append_text(scenario["title"])
        plugin.attach(scenario_combo, 0, 0, 2, 1)
        scenario_combo.set_active(0)

        notebook = Gtk.Notebook()
        notebook.append_page(plugin, Gtk.Label("Scenario"))

        icon = Gtk.Image.new_from_file(ANTARES_ICON)

        sections = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
        sections.pack_start(icon, False, False, 0)
        sections.pack_start(notebook, True, True, 0)
        self.get_content_area().add(sections)

    def set_scenario(self, s):
        self.scenario = s["id"]
        self.download.set_label(s["title"])
        self.download.set_uri(s["download url"])
        self.author.set_label(s["author"])
        self.author.set_uri(s["author url"])
        self.version.set_label(s["version"])

    def on_change_scenario(self, combo):
        self.set_scenario(self.scenarios[combo.get_active()])

    def on_response(self, widget, value):
        if value == Gtk.ResponseType.OK:
            args = [ANTARES_BIN, self.scenario]
            if APP_DATA:
                args += ["--app-data", APP_DATA]
            if FACTORY_SCENARIO:
                args += ["--factory-scenario", FACTORY_SCENARIO]
            print(" ".join(args))
            os.execvp(args[0], args)
        else:
            Gtk.main_quit()


def reinstall_or_check_scenario():
    args = [INSTALL_DATA_BIN]
    args += ["--dest", SCENARIOS]
    if FACTORY_SCENARIO:
        # Running from build dir: reinstall.
        pass
    else:
        # Running from installation: just check.
        args += ["--check"]
    print(" ".join(args))
    subprocess.check_call(args)



def ls_scenarios():
    args = [LS_SCENARIOS_BIN]
    if FACTORY_SCENARIO:
        args += ["--factory-scenario", FACTORY_SCENARIO]
    print(" ".join(args))
    out = subprocess.check_output(args)
    scenarios = []
    for line in out.splitlines():
        if not line[0].isspace():
            scenarios.append({"id": line[:-1]})
        else:
            key, val = line.strip().split(": ", 1)
            scenarios[-1][key] = val
    return scenarios


if __name__ == "__main__":
    main()
