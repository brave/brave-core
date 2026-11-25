@echo off
rem Copyright (c) 2026 The Brave Authors. All rights reserved.
rem This Source Code Form is subject to the terms of the Mozilla Public
rem License, v. 2.0. If a copy of the MPL was not distributed with this file,
rem You can obtain one at https://mozilla.org/MPL/2.0/.

rem Runs the test suites of an isolate archive. All arguments are passed to
rem each test binary. See build/isolate/README.md in brave-core.

setlocal

where node >nul 2>nul
if errorlevel 1 (
  echo error: node is not found in PATH 1>&2
  exit /b 1
)

node "%~dp0runner\check_environment.cjs"
if errorlevel 1 exit /b 1

node "%~dp0runner\run.ts" "%~dp0manifest.json" %*
exit /b %ERRORLEVEL%
