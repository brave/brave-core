/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_NEW_TAB_BUTTON_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_NEW_TAB_BUTTON_H_

#include "chrome/browser/ui/tabs/tab_style.h"
#include "ui/base/metadata/metadata_header_macros.h"

// #define PaintFill                 \
//   UnUsed() {}                     \
//   friend class BraveNewTabButton; \
//   virtual void PaintFill

#define context_menu_model_                            \
  context_menu_model_;                                 \
  METADATA_HEADER(NewTabButton, TabStripControlButton) \
  void Unused()
#include <chrome/browser/ui/views/tabs/new_tab_button.h>  // IWYU pragma: export

#undef context_menu_model_
// #undef PaintFill

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_NEW_TAB_BUTTON_H_
