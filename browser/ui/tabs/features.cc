/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/tabs/features.h"

namespace tabs::features {

#if BUILDFLAG(IS_LINUX)
BASE_FEATURE(kBraveChangeActiveTabOnScrollEvent,
             "BraveChangeActiveTabOnScrollEvent",
             base::FEATURE_ENABLED_BY_DEFAULT);
#endif  // BUILDFLAG(IS_LINUX)

BASE_FEATURE(kBraveSharedPinnedTabs,
             "BraveSharedPinnedTabs",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kBraveHorizontalTabsUpdate,
             "BraveHorizontalTabsUpdate",
             base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE(kBraveCompactHorizontalTabs,
             "BraveCompactHorizontalTabs",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kBraveVerticalTabScrollBar,
             "BraveVerticalTabScrollBar",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kBraveSplitView,
             "BraveSplitView",
             base::FEATURE_DISABLED_BY_DEFAULT);

bool HorizontalTabsUpdateEnabled() {
  return base::FeatureList::IsEnabled(kBraveHorizontalTabsUpdate);
}

}  // namespace tabs::features
