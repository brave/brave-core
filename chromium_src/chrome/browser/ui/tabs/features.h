/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TABS_FEATURES_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TABS_FEATURES_H_

#include <chrome/browser/ui/tabs/features.h>  // IWYU pragma: export

#include "base/feature_list.h"

namespace tabs {

#if BUILDFLAG(IS_LINUX)
// This flag controls the behavior of browser_default::kScrollEventChangesTab,
// which is true only when it's Linux.
BASE_DECLARE_FEATURE(kBraveChangeActiveTabOnScrollEvent);
#endif  // BUILDFLAG(IS_LINUX)

BASE_DECLARE_FEATURE(kBraveSharedPinnedTabs);

BASE_DECLARE_FEATURE(kBraveHorizontalTabsUpdate);

BASE_DECLARE_FEATURE(kBraveCompactHorizontalTabs);

BASE_DECLARE_FEATURE(kBraveVerticalTabScrollBar);

BASE_DECLARE_FEATURE(kBraveVerticalTabHideCompletely);

BASE_DECLARE_FEATURE(kBraveTreeTab);

BASE_DECLARE_FEATURE(kBraveRenamingTabs);

bool HorizontalTabsUpdateEnabled();

}  // namespace tabs

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TABS_FEATURES_H_
