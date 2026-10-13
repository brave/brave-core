// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_PAGE_ACTION_WAYBACK_MACHINE_AUTO_CHECK_H_
#define BRAVE_BROWSER_UI_VIEWS_PAGE_ACTION_WAYBACK_MACHINE_AUTO_CHECK_H_

#include "base/memory/raw_ref.h"
#include "brave/components/brave_wayback_machine/wayback_state.h"

namespace tabs {
class TabInterface;
}

// Runs the automatic Wayback Machine flow for a tab: when enabled, looks up a
// snapshot after a failed navigation in the active tab displays an infobar if
// found.
class WaybackMachineAutoCheck {
 public:
  explicit WaybackMachineAutoCheck(tabs::TabInterface& tab);
  WaybackMachineAutoCheck(const WaybackMachineAutoCheck&) = delete;
  WaybackMachineAutoCheck& operator=(const WaybackMachineAutoCheck&) = delete;
  ~WaybackMachineAutoCheck();

  // Returns whether an automatic lookup is in progress.
  bool is_checking() const { return is_checking_; }

  // Updates the auto-check flow state based on the specified WaybackState.
  void OnWaybackStateChanged(WaybackState state);

  // Resets the current auto-check state, e.g. when the tab's contents are
  // replaced.
  void Reset();

 private:
  bool ShouldCheck();
  void MaybeShowInfoBar();

  const raw_ref<tabs::TabInterface> tab_;
  bool is_checking_ = false;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_PAGE_ACTION_WAYBACK_MACHINE_AUTO_CHECK_H_
