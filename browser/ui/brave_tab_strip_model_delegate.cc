/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_tab_strip_model_delegate.h"

#include "brave/browser/ui/tabs/features.h"
#include "brave/browser/ui/tabs/shared_pinned_tab_service.h"
#include "brave/browser/ui/tabs/shared_pinned_tab_service_factory.h"
#include "chrome/browser/ui/browser.h"

namespace chrome {

bool BraveTabStripModelDelegate::CanMoveTabsToWindow(
    const std::vector<int>& indices) {
  if (!base::FeatureList::IsEnabled(tabs::features::kBraveSharedPinnedTabs)) {
    return BrowserTabStripModelDelegate::CanMoveTabsToWindow(indices);
  }

  // Shared pinned tabs shouldn't be moved.
  return base::ranges::none_of(indices, [this](const auto& index) {
    return browser_->tab_strip_model()->IsTabPinned(index);
  });
}

void BraveTabStripModelDelegate::CacheWebContents(
    const std::vector<std::unique_ptr<DetachedWebContents>>& web_contents) {
  BrowserTabStripModelDelegate::CacheWebContents(web_contents);
  if (!base::FeatureList::IsEnabled(tabs::features::kBraveSharedPinnedTabs)) {
    return;
  }

  auto* shared_pinned_tab_service =
      SharedPinnedTabServiceFactory::GetForProfile(browser_->profile());
  DCHECK(shared_pinned_tab_service);
  shared_pinned_tab_service->CacheWebContentsIfNeeded(browser_, web_contents);
}

}  // namespace chrome
