/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TABS_PUBLIC_TAB_FEATURES_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TABS_PUBLIC_TAB_FEATURES_H_

#include "base/callback_list.h"
#include "brave/browser/brave_screenshots/brave_screenshots_tab_feature.h"

#define TabFeatures TabFeatures_Chromium

#include "src/chrome/browser/ui/tabs/public/tab_features.h"  // IWYU pragma: export

#undef TabFeatures

#include "brave/browser/ui/tabs/public/tab_features.h"

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TABS_PUBLIC_TAB_FEATURES_H_
