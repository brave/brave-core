@echo off
:: Copyright (c) 2026 The Brave Authors. All rights reserved.
:: This Source Code Form is subject to the terms of the Mozilla Public
:: License, v. 2.0. If a copy of the MPL was not distributed with this file,
:: You can obtain one at https://mozilla.org/MPL/2.0/.
::
:: Runs the node delivered into third_party/node for the brave-core checkout the
:: current directory is in, falling back to the system node when there is none.
:: See launcher.py for the resolution logic.
python3 "%~dp0launcher.py" --allow-fallback node-win %*
