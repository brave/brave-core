/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_browser.h"

#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "components/tabs/public/tab_interface.h"

BraveBrowser::BraveBrowser(BrowserWindowCreateParams params)
    : Browser(std::move(params)) {}

BraveBrowser::~BraveBrowser() = default;

void BraveBrowser::OnTabStripModelChanged(
    TabStripModel* tab_strip_model,
    const TabStripModelChange& change,
    const TabStripSelectionChange& selection) {
  Browser::OnTabStripModelChanged(tab_strip_model, change, selection);

  // Check if all tabs we set to ignore onbeforeunload handler are closed.
  if (!tabs_closing_with_onbeforeunload_ignore_.empty() &&
      change.type() == TabStripModelChange::Type::kRemoved) {
    for (auto& removed_tab : change.GetRemove()->contents) {
      tabs_closing_with_onbeforeunload_ignore_.erase(
          removed_tab.tab->GetHandle());
    }
  }
}

void BraveBrowser::SetTabsToIgnoreBeforeUnloadHandlers(
    const base::flat_set<tabs::TabHandle>& for_contents) {
  tabs_closing_with_onbeforeunload_ignore_ = for_contents;
}

bool BraveBrowser::ShouldIgnoreBeforeUnloadHandlerForTab(
    tabs::TabHandle handle) const {
  return tabs_closing_with_onbeforeunload_ignore_.contains(handle);
}
