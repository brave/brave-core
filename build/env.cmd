@echo off
rem Copyright (c) 2023 The Brave Authors. All rights reserved.
rem This Source Code Form is subject to the terms of the Mozilla Public
rem License, v. 2.0. If a copy of the MPL was not distributed with this file,
rem You can obtain one at https://mozilla.org/MPL/2.0/.

for /f "tokens=* delims==" %%i in ('npm run --silent --prefix "%~dp0.." gen_env') do (
  if "%1" == "-v" (
    echo %%i
  )
  set %%i
)
