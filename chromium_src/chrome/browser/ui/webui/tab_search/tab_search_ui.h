// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_TAB_SEARCH_TAB_SEARCH_UI_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_TAB_SEARCH_TAB_SEARCH_UI_H_

#include "brave/browser/ui/webui/tab_search/brave_tab_search.h"
#include "brave/browser/ui/webui/tab_search/brave_tab_search.mojom-forward.h"

#define page_handler_for_testing                                          \
  page_handler_for_testing_unused() {                                     \
    return nullptr;                                                       \
  }                                                                       \
  void BindInterface(                                                     \
      mojo::PendingReceiver<tab_search::mojom::BraveTabSearch> receiver); \
  std::unique_ptr<BraveTabSearch> brave_tab_search_;                      \
  TabSearchPageHandler* page_handler_for_testing

#include "src/chrome/browser/ui/webui/tab_search/tab_search_ui.h"

#undef page_handler_for_testing

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_TAB_SEARCH_TAB_SEARCH_UI_H_
