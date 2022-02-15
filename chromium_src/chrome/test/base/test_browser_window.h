/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_TEST_BASE_TEST_BROWSER_WINDOW_H_
#define BRAVE_CHROMIUM_SRC_CHROME_TEST_BASE_TEST_BROWSER_WINDOW_H_

#include "brave/browser/ui/brave_browser_window.h"

#define BrowserWindow BraveBrowserWindow
#include "src/chrome/test/base/test_browser_window.h"
#undef BrowserWindow

#endif  // BRAVE_CHROMIUM_SRC_CHROME_TEST_BASE_TEST_BROWSER_WINDOW_H_
