/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_NEW_TAB_BUTTON_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_NEW_TAB_BUTTON_H_

#define BRAVE_NEW_TAB_BUTTON_H_   \
 private:                         \
  friend class BraveNewTabButton; \
                                  \
 public:                          \
  // #define BRAVE_NEW_TAB_BUTTON_H_

#define GetBorderPath virtual GetBorderPath
#include "../../../../../../../chrome/browser/ui/views/tabs/new_tab_button.h"
#undef GetBorderPath
#undef BRAVE_NEW_TAB_BUTTON_H_

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_NEW_TAB_BUTTON_H_
