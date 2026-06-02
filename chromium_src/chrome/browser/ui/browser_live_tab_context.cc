// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <map>
#include <string>

#include "base/feature_list.h"
#include "brave/browser/ui/tabs/tree_tab_session_observer.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/tabs/features.h"

namespace {

// Called from BrowserLiveTabContext::GetExtraDataForTab to record the tree-tab
// position of the tab being closed. Writes kBraveTreeNodeIdKey and
// kBraveTreeParentNodeIdKey, and kBraveTreeNodeCollapsedKey into |extra_data|.
void MaybePopulateTreeTabExtraData(
    BrowserWindowInterface& browser,
    int index,
    std::map<std::string, std::string>& extra_data) {
  if (!base::FeatureList::IsEnabled(tabs::kBraveTreeTab)) {
    return;
  }

  auto* tree_tab_session_observer =
      browser.GetFeatures().GetTreeTabSessionObserver();
  if (!tree_tab_session_observer) {
    // Can be null if the browser isn't a normal browser.
    return;
  }

  tree_tab_session_observer->MaybePopulateTreeTabExtraData(index, &extra_data);
}

}  // namespace

#include <chrome/browser/ui/browser_live_tab_context.cc>
