// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_SIDE_PANEL_SIDE_PANEL_UI_BASE_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_SIDE_PANEL_SIDE_PANEL_UI_BASE_H_

class BraveSidePanelCoordinator;

#define set_current_key                   \
  Unused();                               \
  friend class BraveSidePanelCoordinator; \
  void set_current_key

#include <chrome/browser/ui/views/side_panel/side_panel_ui_base.h>  // IWYU pragma: export

#undef set_current_key

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_SIDE_PANEL_SIDE_PANEL_UI_BASE_H_
