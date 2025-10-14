// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/views/side_panel/side_panel_toolbar_pinning_controller.h"

#include "chrome/browser/ui/views/side_panel/side_panel_coordinator.h"

#define UpdateActiveState UpdateActiveState_ChromiumImpl

#include <chrome/browser/ui/views/side_panel/side_panel_toolbar_pinning_controller.cc>

#undef UpdateActiveState

void SidePanelToolbarPinningController::UpdateActiveState(
    SidePanelEntryKey key,
    bool show_active_in_toolbar) {
  if (!browser_view_->toolbar()->pinned_toolbar_actions_container()) {
    return;
  }

  UpdateActiveState_ChromiumImpl(key, show_active_in_toolbar);
}
