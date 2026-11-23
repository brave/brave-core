/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/tabs/features.h"

#include "base/command_line.h"
#include "base/feature_list.h"
#include "brave/browser/ui/tabs/public/switches.h"

#include <chrome/browser/ui/tabs/features.cc>

namespace tabs {

#if BUILDFLAG(IS_LINUX)
BASE_FEATURE(kBraveChangeActiveTabOnScrollEvent,
             base::FEATURE_ENABLED_BY_DEFAULT);
#endif  // BUILDFLAG(IS_LINUX)

BASE_FEATURE(kBraveSharedPinnedTabs, base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kBraveHorizontalTabsUpdate, base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE(kBraveVerticalTabScrollBar, base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kBraveVerticalTabHideCompletely, base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE(kBraveTreeTab, base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE(kBraveScrollableTabStrip, base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE(kBraveBringAllTabsToThisWindow, base::FEATURE_ENABLED_BY_DEFAULT);

bool HorizontalTabsUpdateEnabled() {
  return base::FeatureList::IsEnabled(kBraveHorizontalTabsUpdate);
}

bool IsUpstreamVerticalTabsForceEnabled() {
  // Check for the force-upstream switch used by the vertical tab migration.
  auto* command_line = base::CommandLine::ForCurrentProcess();
  return command_line->HasSwitch(tabs::switches::kVerticalTabMigrationSwitch) &&
         command_line->GetSwitchValueASCII(
             tabs::switches::kVerticalTabMigrationSwitch) ==
             tabs::switches::kVerticalTabMigrationForceUpstreamValue;
}

}  // namespace tabs
