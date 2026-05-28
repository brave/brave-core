// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/ai_chat/tab_activator.h"

#include "base/functional/bind.h"
#include "brave/components/ai_chat/core/browser/tab_tracker_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/tabs/public/tab_interface.h"
#include "ui/base/base_window.h"

namespace ai_chat {

TabActivator::TabActivator(Profile* profile, TabTrackerService* tab_tracker)
    : profile_(profile), tab_tracker_(tab_tracker) {
  if (tab_tracker_) {
    // The activator callback captures `this` via Unretained; we explicitly
    // clear it from the destructor below. We avoid base::WeakPtr here because
    // WeakPtr-bound callbacks may not return a value.
    tab_tracker_->SetActivator(base::BindRepeating(&TabActivator::ActivateTab,
                                                   base::Unretained(this)));
  }
}

TabActivator::~TabActivator() {
  // KeyedService destruction ordering guarantees `tab_tracker_` outlives us
  // (we depend on it), but its stored activator callback would dangle, so
  // clear it.
  if (tab_tracker_) {
    tab_tracker_->SetActivator({});
  }
}

bool TabActivator::ActivateTab(int32_t tab_id) {
  // The ids stored in TabTrackerService come from `tabs::TabInterface::Handle`
  // (see TabDataWebContentsObserver), so we resolve them via the handle's
  // global lookup rather than iterating BrowserList by SessionID.
  tabs::TabInterface* tab = tabs::TabHandle(tab_id).Get();
  if (!tab) {
    return false;
  }
  BrowserWindowInterface* browser = tab->GetBrowserWindowInterface();
  if (!browser || browser->GetProfile() != profile_) {
    return false;
  }
  TabStripModel* tab_strip = browser->GetTabStripModel();
  if (!tab_strip) {
    return false;
  }
  const int index = tab_strip->GetIndexOfTab(tab);
  if (index == TabStripModel::kNoTab) {
    return false;
  }
  tab_strip->ActivateTabAt(index);
  if (ui::BaseWindow* window = browser->GetWindow()) {
    window->Activate();
  }
  return true;
}

}  // namespace ai_chat
