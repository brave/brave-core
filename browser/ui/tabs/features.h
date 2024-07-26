/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_TABS_FEATURES_H_
#define BRAVE_BROWSER_UI_TABS_FEATURES_H_

#include "base/feature_list.h"
namespace tabs::features {

#if BUILDFLAG(IS_LINUX)
// This flag controls the behavior of browser_default::kScrollEventChangesTab,
// which is true only when it's Linux.
BASE_DECLARE_FEATURE(kBraveChangeActiveTabOnScrollEvent);
#endif  // BUILDFLAG(IS_LINUX)

BASE_DECLARE_FEATURE(kBraveSharedPinnedTabs);

BASE_DECLARE_FEATURE(kBraveHorizontalTabsUpdate);

BASE_DECLARE_FEATURE(kBraveCompactHorizontalTabs);

BASE_DECLARE_FEATURE(kBraveVerticalTabScrollBar);

BASE_DECLARE_FEATURE(kBraveSplitView);

bool HorizontalTabsUpdateEnabled();

}  // namespace tabs::features

#endif  // BRAVE_BROWSER_UI_TABS_FEATURES_H_
