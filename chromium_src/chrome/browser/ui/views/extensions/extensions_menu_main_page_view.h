/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_EXTENSIONS_EXTENSIONS_MENU_MAIN_PAGE_VIEW_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_EXTENSIONS_EXTENSIONS_MENU_MAIN_PAGE_VIEW_H_

#define GetActiveWebContents                    \
  GetActiveWebContents_Unused();                \
  friend class BraveExtensionsMenuMainPageView; \
  content::WebContents* GetActiveWebContents

#include "src/chrome/browser/ui/views/extensions/extensions_menu_main_page_view.h"  // IWYU pragma: export

#undef GetActiveWebContents

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_EXTENSIONS_EXTENSIONS_MENU_MAIN_PAGE_VIEW_H_
