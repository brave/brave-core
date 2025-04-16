/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_PAGE_ACTION_WAYBACK_MACHINE_STATE_MANAGER_H_
#define BRAVE_BROWSER_UI_VIEWS_PAGE_ACTION_WAYBACK_MACHINE_STATE_MANAGER_H_

#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"

class Browser;
class WaybackMachineActionIconView;
enum class WaybackState;

// This class listens active tab's wayback state and let action
// icon know about the state change. Action icon owns this class.
class WaybackMachineStateManager : public TabStripModelObserver {
 public:
  WaybackMachineStateManager(WaybackMachineActionIconView* icon,
                             Browser* browser);
  ~WaybackMachineStateManager() override;

  WaybackMachineStateManager(const WaybackMachineStateManager&) = delete;
  WaybackMachineStateManager& operator=(const WaybackMachineStateManager&) =
      delete;

  WaybackState GetActiveTabWaybackState() const;

  // TabStripModelObserver overrides:
  void OnTabStripModelChanged(
      TabStripModel* tab_strip_model,
      const TabStripModelChange& change,
      const TabStripSelectionChange& selection) override;
  void OnTabGroupChanged(const TabGroupChange& change) override;

 private:
  void OnWaybackStateChanged(WaybackState state);

  raw_ref<WaybackMachineActionIconView> icon_;
  raw_ref<Browser> browser_;
  base::WeakPtrFactory<WaybackMachineStateManager> weak_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_PAGE_ACTION_WAYBACK_MACHINE_STATE_MANAGER_H_
