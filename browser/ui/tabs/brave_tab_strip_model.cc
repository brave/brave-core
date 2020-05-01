/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <algorithm>
#include <memory>

#include "brave/browser/ui/tabs/brave_tab_strip_model.h"

#include "brave/common/pref_names.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"

BraveTabStripModel::BraveTabStripModel(TabStripModelDelegate* delegate,
                                       Profile* profile)
    : TabStripModel(delegate, profile) {}

BraveTabStripModel::~BraveTabStripModel() {}

void BraveTabStripModel::SelectRelativeTab(bool next,
                                           UserGestureDetails detail) {
  bool isMRUEnabled = profile()->GetPrefs()->GetBoolean(kMRUCyclingEnabled);

  if (isMRUEnabled) {
    SelectMRUTab(!next, detail);
  } else {
    TabStripModel::SelectRelativeTab(next, detail);
  }
}

void BraveTabStripModel::SelectMRUTab(bool backward,
                                      UserGestureDetails detail) {
  if (current_mru_cycling_index_ == -1) {
    // Start cycling

    // Create a list of tab indexes sorted by time of last activation
    for (int i = 0; i < count(); ++i) {
      mru_cycle_list.push_back(i);
    }

    std::sort(mru_cycle_list.begin(), mru_cycle_list.end(),
              [this](int a, int b) {
                return GetWebContentsAt(a)->GetLastActiveTime() >
                       GetWebContentsAt(b)->GetLastActiveTime();
              });

    current_mru_cycling_index_ = 0;

    // Tell the controllers that we start cycling to handle tabs keys
    for (auto& observer : observers_)
      observer.StartMRUCycling(this);
  }

  int tabCount = mru_cycle_list.size();

  if (tabCount == 0) {
    return;
  }

  int nextCycle = backward ? -1 : 1;

  current_mru_cycling_index_ =
      (current_mru_cycling_index_ + nextCycle % tabCount + tabCount) % tabCount;

  ActivateTabAt(mru_cycle_list[current_mru_cycling_index_], detail);
}

void BraveTabStripModel::StopMRUCycling() {
  current_mru_cycling_index_ = -1;
  mru_cycle_list.clear();
}
