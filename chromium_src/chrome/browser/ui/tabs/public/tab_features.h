/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TABS_PUBLIC_TAB_FEATURES_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TABS_PUBLIC_TAB_FEATURES_H_

#include "base/callback_list.h"

#define CreateTabFeatures() \
  CreateTabFeatures();      \
  static std::unique_ptr<TabFeatures> CreateTabFeatures_ChromiumImpl()

#define ReplaceTabFeaturesForTesting(__VA_ARGS__)         \
  ReplaceTabFeaturesForTesting_ChromiumImpl(__VA_ARGS__); \
  static void ReplaceTabFeaturesForTesting(__VA_ARGS__)

#include "src/chrome/browser/ui/tabs/public/tab_features.h"  // IWYU pragma: export
#undef CreateTabFeatures
#undef ReplaceTabFeaturesForTesting

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TABS_PUBLIC_TAB_FEATURES_H_
