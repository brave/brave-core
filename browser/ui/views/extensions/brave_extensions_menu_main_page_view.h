/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_EXTENSIONS_BRAVE_EXTENSIONS_MENU_MAIN_PAGE_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_EXTENSIONS_BRAVE_EXTENSIONS_MENU_MAIN_PAGE_VIEW_H_

#include "chrome/browser/ui/views/extensions/extensions_menu_main_page_view.h"
#include "ui/base/metadata/metadata_header_macros.h"

class BraveExtensionsMenuMainPageView : public ExtensionsMenuMainPageView {
  METADATA_HEADER(BraveExtensionsMenuMainPageView, ExtensionsMenuMainPageView)

 public:
  BraveExtensionsMenuMainPageView(Browser* browser,
                                  ExtensionsMenuHandler* menu_handler);
  ~BraveExtensionsMenuMainPageView() override;

  // BraveExtensionsMenuMainPageView:
  void OnThemeChanged() override;

 private:
  void UpdateButtons();
};

#endif  // BRAVE_BROWSER_UI_VIEWS_EXTENSIONS_BRAVE_EXTENSIONS_MENU_MAIN_PAGE_VIEW_H_
