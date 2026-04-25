/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TABS_TAB_DATA_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TABS_TAB_DATA_H_

#define FromTabInterface                                  \
  FromTabInterface_ChromiumImpl(tabs::TabInterface* tab); \
  static TabData FromTabInterface

#include <chrome/browser/ui/tabs/tab_data.h>  // IWYU pragma: export
#undef FromTabInterface

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TABS_TAB_DATA_H_
