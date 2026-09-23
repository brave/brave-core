/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_SIDEBAR_SIDEBAR_H_
#define BRAVE_BROWSER_UI_SIDEBAR_SIDEBAR_H_

#include "brave/components/sidebar/browser/sidebar_service.h"

namespace sidebar {

// Interact with UI layer.
class Sidebar {
 public:
  virtual void SetSidebarShowOption(
      SidebarService::ShowSidebarOption show_option) = 0;

  // Update sidebar item's UI state.
  virtual void UpdateSidebarItemsState() = 0;

  // Re-evaluate sidebar visibility from the current pinned state and show
  // option, and sync the toolbar button highlight to the pinned state.
  // Called when the controller's pinned state toggles (V2 only).
  virtual void UpdateSidebarVisibility() = 0;

 protected:
  virtual ~Sidebar() {}
};

}  // namespace sidebar

#endif  // BRAVE_BROWSER_UI_SIDEBAR_SIDEBAR_H_
