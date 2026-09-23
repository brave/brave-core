@echo off
rem Copyright (c) 2026 The Brave Authors. All rights reserved.
rem This Source Code Form is subject to the terms of the Mozilla Public
rem License, v. 2.0. If a copy of the MPL was not distributed with this file,
rem You can obtain one at https://mozilla.org/MPL/2.0/.

setlocal

call python3 -u -B "%~dp0npm_wrapper.py" %*
