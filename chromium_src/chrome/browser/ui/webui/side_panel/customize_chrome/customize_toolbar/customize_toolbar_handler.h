// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_SIDE_PANEL_CUSTOMIZE_CHROME_CUSTOMIZE_TOOLBAR_CUSTOMIZE_TOOLBAR_HANDLER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_SIDE_PANEL_CUSTOMIZE_CHROME_CUSTOMIZE_TOOLBAR_CUSTOMIZE_TOOLBAR_HANDLER_H_

#include "chrome/browser/ui/webui/side_panel/customize_chrome/customize_toolbar/customize_toolbar.mojom.h"

#define ListCategories(...)                 \
  ListCategories_ChromiumImpl(__VA_ARGS__); \
  void ListCategories(__VA_ARGS__)

#define ListActions(...)                 \
  ListActions_ChromiumImpl(__VA_ARGS__); \
  void ListActions(__VA_ARGS__)

#define PinAction(...)                                          \
  PinAction_ChromiumImpl(__VA_ARGS__);                          \
  void ObserveBraveActions();                                   \
  void OnBraveActionPinnedChanged(                              \
      side_panel::customize_chrome::mojom::ActionId action_id); \
  void PinAction(__VA_ARGS__)

#include <chrome/browser/ui/webui/side_panel/customize_chrome/customize_toolbar/customize_toolbar_handler.h>  // IWYU pragma: export

#undef PinAction
#undef ListActions
#undef ListCategories

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_SIDE_PANEL_CUSTOMIZE_CHROME_CUSTOMIZE_TOOLBAR_CUSTOMIZE_TOOLBAR_HANDLER_H_
