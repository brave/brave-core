@echo off
:: Copyright (c) 2026 The Brave Authors. All rights reserved.
:: This Source Code Form is subject to the terms of the Mozilla Public
:: License, v. 2.0. If a copy of the MPL was not distributed with this file,
:: You can obtain one at https://mozilla.org/MPL/2.0/.
::
:: Runs the node delivered into third_party/node for the brave-core checkout the
:: current directory is in, falling back to the system node when there is none.
:: See launcher.py for the resolution logic.
::
:: `%~dp0` is normally this script's directory, but when the shim is invoked as
:: a bare `node` by another process (e.g. npm running a package's scripts) cmd
:: can expand it to the current directory instead. If runner.py is not there,
:: resolve our own name on %PATH% (`%~dp$PATH:0`) to find it beside the shim.
setlocal
set "_dir=%~dp0"
if not exist "%_dir%runner.py" set "_dir=%~dp$PATH:0"
set "_python="
for /f "usebackq delims=" %%i in (`python3 "%_dir%runner.py"`) do set "_python=%%i"
if not defined _python exit /b 1
"%_python%" "%_dir%launcher.py" --allow-fallback node-win %*
