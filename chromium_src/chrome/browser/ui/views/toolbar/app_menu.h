/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TOOLBAR_APP_MENU_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TOOLBAR_APP_MENU_H_

#define SetTimerForTesting                         \
  UnusedMethod1() {}                               \
  friend class BraveAppMenu;                       \
                                                   \
  std::unique_ptr<views::Background>               \
  CreateInMenuButtonBackgroundWithLeadingBorder(); \
                                                   \
  void SetTimerForTesting

#define RunMenu virtual RunMenu

#include <chrome/browser/ui/views/toolbar/app_menu.h>  // IWYU pragma: export

#undef SetTimerForTesting
#undef RunMenu

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TOOLBAR_APP_MENU_H_
