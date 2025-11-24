// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_SIDE_PANEL_SIDE_PANEL_UI_BASE_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_SIDE_PANEL_SIDE_PANEL_UI_BASE_H_

class BraveSidePanelCoordinator;

// Make BraveSidePanelCoordinator a friend of SidePanelUIBase because we want
// to override some of its protected and private methods and have access to
// the private current_key_ member variable.
// Inheritance:
// BraveSidePanelCoordinator -> SidePanelCoordinator -> SidePanelUIBase
#define SetCurrentKey                     \
  Unused();                               \
  friend class BraveSidePanelCoordinator; \
  void SetCurrentKey

#include <chrome/browser/ui/views/side_panel/side_panel_ui_base.h>  // IWYU pragma: export

#undef SetCurrentKey

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_SIDE_PANEL_SIDE_PANEL_UI_BASE_H_
