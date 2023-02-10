/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_SESSION_CRASHED_BUBBLE_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_SESSION_CRASHED_BUBBLE_H_

#define ShowIfNotOffTheRecordProfile                                           \
  ShowIfNotOffTheRecordProfileBrave(Browser* browser, bool skip_tab_checking); \
  static void ShowIfNotOffTheRecordProfile

#include "src/chrome/browser/ui/session_crashed_bubble.h"  // IWYU pragma: export

#undef ShowIfNotOffTheRecordProfile

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_SESSION_CRASHED_BUBBLE_H_
