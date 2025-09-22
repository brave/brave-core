// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_BROWSER_WEBUI_BROWSER_SIDE_PANEL_UI_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_BROWSER_WEBUI_BROWSER_SIDE_PANEL_UI_H_

#include "chrome/browser/ui/views/side_panel/side_panel_ui.h"

// This override for `Toggle()` is being added to `WebUIBrowserSidePanelUI` to
// appease build failures we are getting due to Webium having its own side panel
// now, that also derives from the same base class. For now, we can just provide
// an unimplemented version of `Toggle()`, but this may change if we decide to
// use Webium.
#define Toggle       \
  Toggle() override; \
  void Toggle

#include <chrome/browser/ui/webui_browser/webui_browser_side_panel_ui.h>  // IWYU pragma: export

#undef Toggle

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_BROWSER_WEBUI_BROWSER_SIDE_PANEL_UI_H_
