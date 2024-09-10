/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TABS_PUBLIC_TAB_FEATURES_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TABS_PUBLIC_TAB_FEATURES_H_

// Prevent PRESUBMIT from complaining about including an std header but not
// using std:: in the file: no-std-usage-because-pch-file
#include <vector>  // vector->locale->ios has an Init.

#define Init(...)                 \
  Init_ChromiumImpl(__VA_ARGS__); \
  void Init(__VA_ARGS__)

#include "src/chrome/browser/ui/tabs/public/tab_features.h"  // IWYU pragma: export
#undef Init

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TABS_PUBLIC_TAB_FEATURES_H_
