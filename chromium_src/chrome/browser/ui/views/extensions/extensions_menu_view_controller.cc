/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/extensions/brave_extensions_menu_main_page_view.h"

#define BRAVE_EXTENSIONS_MENU_VIEW_CONTROLLER_OPEN_MAIN_PAGE               \
  {                                                                        \
    auto main_page =                                                       \
        std::make_unique<BraveExtensionsMenuMainPageView>(browser_, this); \
    UpdateMainPage(main_page.get(), GetActiveWebContents());               \
    PopulateMainPage(main_page.get());                                     \
    SwitchToPage(std::move(main_page));                                    \
    return;                                                                \
  }

#include "src/chrome/browser/ui/views/extensions/extensions_menu_view_controller.cc"

#undef BRAVE_EXTENSIONS_MENU_VIEW_CONTROLLER_OPEN_MAIN_PAGE
