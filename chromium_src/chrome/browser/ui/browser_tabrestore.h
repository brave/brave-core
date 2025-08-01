// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_BROWSER_TABRESTORE_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_BROWSER_TABRESTORE_H_

// Add decorator method to manage custom tab title when adding restored tabs.
#define AddRestoredTab(...)                 \
  AddRestoredTab_ChromiumImpl(__VA_ARGS__); \
  content::WebContents* AddRestoredTab(__VA_ARGS__)

// Add decorator method to manage custom tab title when replacing restored tabs.
#define ReplaceRestoredTab(...)                 \
  ReplaceRestoredTab_ChromiumImpl(__VA_ARGS__); \
  content::WebContents* ReplaceRestoredTab(__VA_ARGS__)

#include <chrome/browser/ui/browser_tabrestore.h>  // IWYU pragma: export

#undef ReplaceRestoredTab
#undef AddRestoredTab

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_BROWSER_TABRESTORE_H_
