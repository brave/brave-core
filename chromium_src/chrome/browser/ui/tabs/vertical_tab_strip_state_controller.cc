/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <chrome/browser/ui/tabs/vertical_tab_strip_state_controller.cc>

namespace tabs {

bool VerticalTabStripStateController::ShouldHideCompletelyWhenCollapsed()
    const {
  return false;
}

}  // namespace tabs
