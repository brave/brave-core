/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_BOOKMARKS_BOOKMARK_MENU_DELEGATE_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_BOOKMARKS_BOOKMARK_MENU_DELEGATE_H_

#include "chrome/browser/ui/views/bookmarks/bookmark_menu_controller_views.h"
#include "ui/views/controls/menu/menu_delegate.h"

// Adjust maximum menu width to fit German localization.
#define GetMaxWidthForMenu(...)           \
  GetMaxWidthForMenu_UnUsed(__VA_ARGS__); \
  int GetMaxWidthForMenu(__VA_ARGS__)

#include <chrome/browser/ui/views/bookmarks/bookmark_menu_delegate.h>  // IWYU pragma: export
#undef GetMaxWidthForMenu

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_BOOKMARKS_BOOKMARK_MENU_DELEGATE_H_
