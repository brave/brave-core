/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_tab_strip_model_delegate.h"

#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/tabs/features.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "components/prefs/pref_service.h"

namespace chrome {

bool BraveTabStripModelDelegate::CanMoveTabsToWindow(
    const std::vector<int>& indices) {
  if (!base::FeatureList::IsEnabled(tabs::features::kBraveSharedPinnedTabs) ||
      !browser_->profile()->GetPrefs()->GetBoolean(
          brave_tabs::kSharedPinnedTab)) {
    return BrowserTabStripModelDelegate::CanMoveTabsToWindow(indices);
  }

  // Shared pinned tabs shouldn't be moved.
  return base::ranges::none_of(indices, [this](const auto& index) {
    return browser_->tab_strip_model()->IsTabPinned(index);
  });
}

}  // namespace chrome
