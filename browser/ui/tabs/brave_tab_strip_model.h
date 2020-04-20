/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_TABS_BRAVE_TAB_STRIP_MODEL_H_
#define BRAVE_BROWSER_UI_TABS_BRAVE_TAB_STRIP_MODEL_H_

#include <memory>
#include <vector>

#include "chrome/browser/ui/tabs/tab_strip_model.h"

#include "ui/events/event_handler.h"

class BraveTabStripModel : public TabStripModel {
 public:
  explicit BraveTabStripModel(TabStripModelDelegate* delegate,
                              Profile* profile);

  ~BraveTabStripModel() override;

  void SelectNextTab(UserGestureDetails detail =
                         UserGestureDetails(GestureType::kOther)) override;

  void SelectPreviousTab(UserGestureDetails detail =
                             UserGestureDetails(GestureType::kOther)) override;

  // Set the next tab when doing a MRU cycling with Ctrl-tab
  void SelectTabMRU(
      bool backward,
      UserGestureDetails detail = UserGestureDetails(GestureType::kOther));

  // Stop MRU cycling, called when releasing the Ctrl key
  void StopMRUCycling();

 private:
  // Current index when we are MRU cycling, set to -1 when not cycling
  int current_mru_cycling_index = -1;

  // List of tab indexes sorted by most recently used
  std::vector<int> mru_cycle_list;
};

#endif  // BRAVE_BROWSER_UI_TABS_BRAVE_TAB_STRIP_MODEL_H_
