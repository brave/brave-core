/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TOOLBAR_APP_MENU_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TOOLBAR_APP_MENU_H_

#define RunMenu      \
  UnusedMethod1() {} \
  virtual void RunMenu

#define AddMenuItem                   \
  UnusedMethod2() { return nullptr; } \
  friend class BraveAppMenu;          \
  virtual views::MenuItemView* AddMenuItem

#include "src/chrome/browser/ui/views/toolbar/app_menu.h"  // IWYU pragma: export

#undef RunMenu
#undef AddMenuItem

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TOOLBAR_APP_MENU_H_
