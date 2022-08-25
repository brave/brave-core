// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_SIDE_SEARCH_SIDE_SEARCH_BROWSER_CONTROLLER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_SIDE_SEARCH_SIDE_SEARCH_BROWSER_CONTROLLER_H_

#include "brave/browser/ui/views/side_panel/brave_side_panel.h"
#include "chrome/browser/ui/views/side_panel/side_panel.h"

#define SidePanel BraveSidePanel
#include "src/chrome/browser/ui/views/side_search/side_search_browser_controller.h"
#undef SidePanel

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_SIDE_SEARCH_SIDE_SEARCH_BROWSER_CONTROLLER_H_
