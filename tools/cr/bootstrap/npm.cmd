@echo off
:: Copyright (c) 2026 The Brave Authors. All rights reserved.
:: This Source Code Form is subject to the terms of the Mozilla Public
:: License, v. 2.0. If a copy of the MPL was not distributed with this file,
:: You can obtain one at https://mozilla.org/MPL/2.0/.
::
:: Runs the npm delivered into third_party/node for the brave-core checkout the
:: current directory is in, falling back to the system npm when there is none.
:: See launcher.py for the resolution logic.
::
:: `%~dp0` is normally this script's directory, but when the shim is invoked as
:: a bare `npm` by another process (e.g. npm running a package's scripts) cmd
:: can expand it to the current directory instead. If launcher.py is not there,
:: resolve our own name on %PATH% (`%~dp$PATH:0`) to find it beside the shim.
setlocal
set "_dir=%~dp0"
if not exist "%_dir%launcher.py" set "_dir=%~dp$PATH:0"
python3 "%_dir%launcher.py" --allow-fallback npm-win %*
