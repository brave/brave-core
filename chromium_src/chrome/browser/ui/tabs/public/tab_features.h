/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TABS_PUBLIC_TAB_FEATURES_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TABS_PUBLIC_TAB_FEATURES_H_

#include "base/callback_list.h"

#define Init virtual Init
#define thumbnail_tab_helper_ \
  thumbnail_tab_helper_;      \
  friend class BraveTabFeatures

#include <chrome/browser/ui/tabs/public/tab_features.h>  // IWYU pragma: export

#undef thumbnail_tab_helper_
#undef Init

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TABS_PUBLIC_TAB_FEATURES_H_
