/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TOOLBAR_APP_MENU_MODEL_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TOOLBAR_APP_MENU_MODEL_H_

#include "brave/browser/ui/toolbar/brave_bookmark_sub_menu_model.h"
#include "chrome/browser/ui/toolbar/bookmark_sub_menu_model.h"

#define CreateZoomMenu                                         \
  CreateZoomMenu_Unused();                                     \
  std::vector<std::unique_ptr<SimpleMenuModel>>& sub_menus() { \
    return sub_menus_;                                         \
  }                                                            \
  void CreateZoomMenu

#define BookmarkSubMenuModel BraveBookmarkSubMenuModel
#include "src/chrome/browser/ui/toolbar/app_menu_model.h"  // IWYU pragma: export
#undef BookmarkSubMenuModel
#undef CreateZoomMenu

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TOOLBAR_APP_MENU_MODEL_H_
