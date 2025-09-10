/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_BROWSER_TAB_STRIP_CONTROLLER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_BROWSER_TAB_STRIP_CONTROLLER_H_

#define ExecuteCommandForTab(...)            \
  virtual ExecuteCommandForTab(__VA_ARGS__); \
  friend class BraveBrowserTabStripController

#include <chrome/browser/ui/views/tabs/browser_tab_strip_controller.h>  // IWYU pragma: export

#undef ExecuteCommandForTab

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_BROWSER_TAB_STRIP_CONTROLLER_H_
