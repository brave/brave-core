// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_SIDE_PANEL_SIDE_PANEL_TOOLBAR_PINNING_CONTROLLER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_SIDE_PANEL_SIDE_PANEL_TOOLBAR_PINNING_CONTROLLER_H_

#define UpdateActiveState(...)                 \
  UpdateActiveState_ChromiumImpl(__VA_ARGS__); \
  void UpdateActiveState(__VA_ARGS__)

#include <chrome/browser/ui/views/side_panel/side_panel_toolbar_pinning_controller.h>  // IWYU pragma: export

#undef UpdateActiveState

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_SIDE_PANEL_SIDE_PANEL_TOOLBAR_PINNING_CONTROLLER_H_
