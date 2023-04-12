/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_DOWNLOAD_DOWNLOAD_SHELF_CONTEXT_MENU_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_DOWNLOAD_DOWNLOAD_SHELF_CONTEXT_MENU_H_

#define GetMenuModel \
  UnUsed() {         \
    return nullptr;  \
  }                  \
  virtual ui::SimpleMenuModel* GetMenuModel

#include "src/chrome/browser/download/download_shelf_context_menu.h"  // IWYU pragma: export

#undef GetMenuModel

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_DOWNLOAD_DOWNLOAD_SHELF_CONTEXT_MENU_H_
