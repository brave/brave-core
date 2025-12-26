// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_BROWSER_UI_PREFS_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_BROWSER_UI_PREFS_H_

#define RegisterBrowserUserPrefs(...)                 \
  RegisterBrowserUserPrefs_ChromiumImpl(__VA_ARGS__); \
  void RegisterBrowserUserPrefs(__VA_ARGS__)

#include <chrome/browser/ui/browser_ui_prefs.h>  // IWYU pragma: export

#undef RegisterBrowserUserPrefs

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_BROWSER_UI_PREFS_H_
