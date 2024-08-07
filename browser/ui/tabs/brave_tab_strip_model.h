/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_TABS_BRAVE_TAB_STRIP_MODEL_H_
#define BRAVE_BROWSER_UI_TABS_BRAVE_TAB_STRIP_MODEL_H_

#include <cstdint>
#include <vector>

#include "base/containers/span.h"
#include "chrome/browser/ui/tabs/tab_enums.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"

class BraveTabStripModel : public TabStripModel {
 public:
  explicit BraveTabStripModel(TabStripModelDelegate* delegate,
                              Profile* profile,
                              TabGroupModelFactory* group_model_factory);

  ~BraveTabStripModel() override;

  BraveTabStripModel(const BraveTabStripModel&) = delete;
  BraveTabStripModel operator=(const BraveTabStripModel&) = delete;

  // Set the next tab when doing a MRU cycling with Ctrl-tab
  void SelectMRUTab(
      TabRelativeDirection direction,
      TabStripUserGestureDetails detail = TabStripUserGestureDetails(
          TabStripUserGestureDetails::GestureType::kOther));

  // Stop MRU cycling, called when releasing the Ctrl key
  void StopMRUCycling();

  // Exposes a |TabStripModel| api to |BraveTabMenuModel|.
  std::vector<int> GetTabIndicesForCommandAt(int tab_index);

  // Closes the tabs at the specified indices.
  void CloseTabs(
      base::span<int> indices,
      uint32_t close_flags = TabCloseTypes::CLOSE_CREATE_HISTORICAL_TAB);

  // TabStripModel:
  void SelectRelativeTab(TabRelativeDirection direction,
                         TabStripUserGestureDetails detail) override;
  void ExecuteContextMenuCommand(int context_index,
                                 ContextMenuCommand command_id) override;

 private:
  // List of tab indexes sorted by most recently used
  std::vector<int> mru_cycle_list_;
};

#endif  // BRAVE_BROWSER_UI_TABS_BRAVE_TAB_STRIP_MODEL_H_
