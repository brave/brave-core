/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/tabs/public/before_unload_ignore_controller.h"

#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"

DEFINE_USER_DATA(BeforeUnloadIgnoreController);

BeforeUnloadIgnoreController::BeforeUnloadIgnoreController(
    BrowserWindowInterface& browser)
    : scoped_unowned_user_data_(browser.GetUnownedUserDataHost(), *this) {
  browser.GetTabStripModel()->AddObserver(this);
}

BeforeUnloadIgnoreController::~BeforeUnloadIgnoreController() = default;

// static
BeforeUnloadIgnoreController* BeforeUnloadIgnoreController::From(
    BrowserWindowInterface* browser) {
  return Get(browser->GetUnownedUserDataHost());
}

void BeforeUnloadIgnoreController::SetTabsToIgnoreBeforeUnloadHandlers(
    const base::flat_set<tabs::TabHandle>& tabs) {
  tabs_to_ignore_ = tabs;
}

bool BeforeUnloadIgnoreController::ShouldIgnoreBeforeUnloadHandlerForTab(
    tabs::TabHandle handle) const {
  return tabs_to_ignore_.contains(handle);
}

void BeforeUnloadIgnoreController::OnTabStripModelChanged(
    TabStripModel* tab_strip_model,
    const TabStripModelChange& change,
    const TabStripSelectionChange& selection) {
  if (tabs_to_ignore_.empty() ||
      change.type() != TabStripModelChange::kRemoved) {
    return;
  }

  for (const auto& removed_tab : change.GetRemove()->contents) {
    tabs_to_ignore_.erase(removed_tab.tab->GetHandle());
  }
}
