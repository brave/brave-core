@echo off
:: Copyright (c) 2026 The Brave Authors. All rights reserved.
:: This Source Code Form is subject to the terms of the Mozilla Public
:: License, v. 2.0. If a copy of the MPL was not distributed with this file,
:: You can obtain one at https://mozilla.org/MPL/2.0/.
::
:: Runs tools/cr/fetch_brave.py from this bootstrap's own checkout to
:: set up a brand-new checkout under the current directory. See launcher.py for
:: the resolution logic.
python3 "%~dp0launcher.py" fetch_brave %*
