/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/tabs/features.h"

#include "chrome/browser/ui/ui_features.h"

namespace tabs::features {

#if BUILDFLAG(IS_LINUX)
BASE_FEATURE(kBraveChangeActiveTabOnScrollEvent,
             base::FEATURE_ENABLED_BY_DEFAULT);
#endif  // BUILDFLAG(IS_LINUX)

BASE_FEATURE(kBraveSharedPinnedTabs,
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kBraveHorizontalTabsUpdate,
             base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE(kBraveCompactHorizontalTabs,
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kBraveVerticalTabScrollBar,
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kBraveVerticalTabHideCompletely,
             base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE(kBraveTreeTab, base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kBraveRenamingTabs,
             base::FEATURE_DISABLED_BY_DEFAULT);

bool HorizontalTabsUpdateEnabled() {
  return base::FeatureList::IsEnabled(kBraveHorizontalTabsUpdate);
}

}  // namespace tabs::features
