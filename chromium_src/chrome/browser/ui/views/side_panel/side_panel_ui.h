// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_SIDE_PANEL_SIDE_PANEL_UI_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_SIDE_PANEL_SIDE_PANEL_UI_H_

#define Toggle  \
  Toggle() = 0; \
  virtual void Toggle

#include "src/chrome/browser/ui/views/side_panel/side_panel_ui.h"  // IWYU pragma: export

#undef Toggle

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_SIDE_PANEL_SIDE_PANEL_UI_H_
