@echo off
:: Copyright (c) 2026 The Brave Authors. All rights reserved.
:: This Source Code Form is subject to the terms of the Mozilla Public
:: License, v. 2.0. If a copy of the MPL was not distributed with this file,
:: You can obtain one at https://mozilla.org/MPL/2.0/.
::
:: Dispatches to tools/cr/plaster.py for the brave-core checkout the current
:: directory is in. See launcher.py for the resolution logic.
::
:: `%~dp0` can expand to the current directory when the shim is invoked as a
:: bare name by another process; if launcher.py is not there, resolve our own
:: name on %PATH% (`%~dp$PATH:0`) to find it beside the shim.
setlocal
set "_dir=%~dp0"
if not exist "%_dir%launcher.py" set "_dir=%~dp$PATH:0"
python3 "%_dir%launcher.py" plaster %*
