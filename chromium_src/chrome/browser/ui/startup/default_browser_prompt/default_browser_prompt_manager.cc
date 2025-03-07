/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/startup/default_browser_prompt/default_browser_prompt_manager.h"

// Don't display the default browser prompt menu item
#define show_app_menu_item_      \
  if (show) {                    \
    show_app_menu_item_ = false; \
    return;                      \
  }                              \
  show_app_menu_item_

#include "src/chrome/browser/ui/startup/default_browser_prompt/default_browser_prompt_manager.cc"

#undef show_app_menu_item_
