/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_LIFETIME_BROWSER_CLOSE_MANAGER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_LIFETIME_BROWSER_CLOSE_MANAGER_H_

#define StartClosingBrowsers           \
  StartClosingBrowsers();              \
  static bool BrowserClosingStarted(); \
  void StartClosingBrowsers_ChromiumImpl

#define CancelBrowserClose \
  CancelBrowserClose();    \
  void CancelBrowserClose_ChromiumImpl

#include "src/chrome/browser/lifetime/browser_close_manager.h"  // IWYU pragma: export

#undef CancelBrowserClose
#undef StartClosingBrowsers

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_LIFETIME_BROWSER_CLOSE_MANAGER_H_
