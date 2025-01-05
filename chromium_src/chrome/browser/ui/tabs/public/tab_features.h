/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TABS_PUBLIC_TAB_FEATURES_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TABS_PUBLIC_TAB_FEATURES_H_

#include "base/callback_list.h"
#include "brave/browser/brave_screenshots/screenshots_tab_feature.h"

#define Init(...)                                                \
  Init_ChromiumImpl(__VA_ARGS__);                                \
                                                                 \
  std::unique_ptr<brave_screenshots::BraveScreenshotsTabFeature> \
      brave_screenshots_tab_feature_;                            \
  brave_screenshots::BraveScreenshotsTabFeature*                 \
  brave_screenshots_tab_feature() {                              \
    return brave_screenshots_tab_feature_.get();                 \
  }                                                              \
                                                                 \
  virtual void Init(__VA_ARGS__)

#include "src/chrome/browser/ui/tabs/public/tab_features.h"  // IWYU pragma: export
#undef Init

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TABS_PUBLIC_TAB_FEATURES_H_
