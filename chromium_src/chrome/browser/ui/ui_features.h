/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_UI_FEATURES_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_UI_FEATURES_H_

#define HasTabSearchToolbarButton \
  HasTabSearchToolbarButton();    \
  bool HasTabSearchToolbarButton_ChromiumImpl

#include <chrome/browser/ui/ui_features.h>  // IWYU pragma: export

#undef HasTabSearchToolbarButton

namespace features {

// A feature flag to force all popup windows to be opened as tabs.
// https://github.com/brave/brave-browser/issues/40959
BASE_DECLARE_FEATURE(kForcePopupToBeOpenedAsTab);

}  // namespace features

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_UI_FEATURES_H_
