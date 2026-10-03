/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/tabs/vertical_tab_strip_state_controller.h"

#include "base/feature_list.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "chrome/browser/ui/tabs/features.h"
#include "components/prefs/pref_service.h"

#include <chrome/browser/ui/tabs/vertical_tab_strip_state_controller.cc>

namespace tabs {

bool VerticalTabStripStateController::ShouldHideCompletelyWhenCollapsed()
    const {
  return base::FeatureList::IsEnabled(tabs::kBraveVerticalTabHideCompletely) &&
         pref_service_->FindPreference(
             brave_tabs::kVerticalTabsHideCompletelyWhenCollapsed) &&
         pref_service_->GetBoolean(
             brave_tabs::kVerticalTabsHideCompletelyWhenCollapsed);
}

}  // namespace tabs
