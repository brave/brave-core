/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_vertical_tab_utils.h"

#include "base/feature_list.h"
#include "brave/browser/ui/tabs/features.h"

bool IsVerticalTabsEnabled() {
  return base::FeatureList::IsEnabled(features::kBraveVerticalTabs);
}

bool ShouldShowVerticalTabs() {
  // TODO(sangwoo.ko) This should consider pref too.
  return IsVerticalTabsEnabled();
}
