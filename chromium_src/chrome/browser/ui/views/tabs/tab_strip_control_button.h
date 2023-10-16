/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_STRIP_CONTROL_BUTTON_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_STRIP_CONTROL_BUTTON_H_

#define UpdateInkDrop virtual UpdateInkDrop
#define GetForegroundColor virtual GetForegroundColor

#include "src/chrome/browser/ui/views/tabs/tab_strip_control_button.h"  // IWYU pragma: export

#undef GetForegroundColor
#undef UpdateInkDrop

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_STRIP_CONTROL_BUTTON_H_
