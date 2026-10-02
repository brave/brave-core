/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/tabs/vertical_tab_strip_state_controller.h"

#include "base/feature_list.h"
#include "chrome/browser/ui/tabs/features.h"
#include "components/prefs/pref_service.h"

#include <chrome/browser/ui/tabs/vertical_tab_strip_state_controller.cc>

// Defined in //brave/browser/ui/views/tabs/vertical_tab_utils.cc.
bool BraveShouldHideVerticalTabsCompletely(PrefService* prefs);

namespace tabs {

bool VerticalTabStripStateController::ShouldHideCompletelyWhenCollapsed()
    const {
  return base::FeatureList::IsEnabled(tabs::kBraveVerticalTabHideCompletely) &&
         BraveShouldHideVerticalTabsCompletely(pref_service_);
}

}  // namespace tabs
