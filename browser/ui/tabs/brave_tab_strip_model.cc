/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/tabs/brave_tab_strip_model.h"

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <vector>

#include "base/containers/span.h"
#include "base/ranges/algorithm.h"
#include "brave/browser/ui/brave_browser_window.h"
#include "brave/components/constants/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/tabs/tab_strip_model_delegate.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"

BraveTabStripModel::BraveTabStripModel(
    TabStripModelDelegate* delegate,
    Profile* profile,
    TabGroupModelFactory* group_model_factory)
    : TabStripModel(delegate, profile, group_model_factory) {}
BraveTabStripModel::~BraveTabStripModel() = default;

void BraveTabStripModel::SelectRelativeTab(TabRelativeDirection direction,
                                           TabStripUserGestureDetails detail) {
  if (GetTabCount() == 0) {
    return;
  }

  bool is_mru_enabled = profile()->GetPrefs()->GetBoolean(kMRUCyclingEnabled);

  if (is_mru_enabled) {
    SelectMRUTab(direction, detail);
  } else {
    TabStripModel::SelectRelativeTab(direction, detail);
  }
}

void BraveTabStripModel::SelectMRUTab(TabRelativeDirection direction,
                                      TabStripUserGestureDetails detail) {
  if (mru_cycle_list_.empty()) {
    // Start cycling

    Browser* browser = chrome::FindBrowserWithTab(GetWebContentsAt(0));
    if (!browser) {
      return;
    }

    // Create a list of tab indexes sorted by time of last activation
    for (int i = 0; i < count(); ++i) {
      mru_cycle_list_.push_back(i);
    }

    std::sort(mru_cycle_list_.begin(), mru_cycle_list_.end(),
              [this](int a, int b) {
                return GetWebContentsAt(a)->GetLastActiveTimeTicks() >
                       GetWebContentsAt(b)->GetLastActiveTimeTicks();
              });

    // Tell the cycling controller that we start cycling to handle tabs keys
    static_cast<BraveBrowserWindow*>(browser->window())->StartTabCycling();
  }

  if (direction == TabRelativeDirection::kNext) {
    std::rotate(mru_cycle_list_.begin(), mru_cycle_list_.begin() + 1,
                mru_cycle_list_.end());
  } else {
    std::rotate(mru_cycle_list_.rbegin(), mru_cycle_list_.rbegin() + 1,
                mru_cycle_list_.rend());
  }

  ActivateTabAt(mru_cycle_list_[0], detail);
}

void BraveTabStripModel::StopMRUCycling() {
  mru_cycle_list_.clear();
}

std::vector<int> BraveTabStripModel::GetTabIndicesForCommandAt(int tab_index) {
  return TabStripModel::GetIndicesForCommand(tab_index);
}

void BraveTabStripModel::CloseTabs(base::span<int> indices,
                                   uint32_t close_types) {
  std::vector<content::WebContents*> contentses;
  for (const auto& index : indices) {
    contentses.push_back(GetWebContentsAt(index));
  }
  TabStripModel::CloseTabs(contentses, close_types);
}
