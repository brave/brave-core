@echo off
:: Copyright (c) 2026 The Brave Authors. All rights reserved.
:: This Source Code Form is subject to the terms of the Mozilla Public
:: License, v. 2.0. If a copy of the MPL was not distributed with this file,
:: You can obtain one at https://mozilla.org/MPL/2.0/.
::
:: Dispatches to tools/cr/alias/cmd.py for the brave-core checkout the current
:: directory is in. See launcher.py for the resolution logic.
::
:: Named `git-cr` so that git resolves `git cr` to this shim via %PATH%, making
:: the `git cr` subcommand work without registering a per-repository alias.
python3 "%~dp0launcher.py" git-cr %*
