/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/tabs/brave_tab_layout_constants.h"

#include "base/feature_list.h"
#include "brave/browser/ui/tabs/features.h"

namespace brave_tabs {

namespace {

bool UseCompact() {
  return base::FeatureList::IsEnabled(
      tabs::features::kBraveCompactHorizontalTabs);
}

}  // namespace

int GetHorizontalTabHeight() {
  return UseCompact() ? 28 : 32;
}

int GetHorizontalTabStripHeight() {
  return GetHorizontalTabHeight() + (kHorizontalTabVerticalSpacing * 2);
}

int GetHorizontalTabPadding() {
  return UseCompact() ? 4 : 8;
}

int GetTabGroupTitleVerticalInset() {
  return (GetHorizontalTabHeight() - kTabGroupLineHeight) / 2;
}

int GetTabGroupTitleHorizontalInset() {
  return UseCompact() ? 6 : 10;
}

}  // namespace brave_tabs
