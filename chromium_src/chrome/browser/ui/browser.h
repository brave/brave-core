/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_BROWSER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_BROWSER_H_

#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "chrome/browser/ui/unload_controller.h"

#define FinishWarnBeforeClosing virtual FinishWarnBeforeClosing
#define ScheduleUIUpdate virtual ScheduleUIUpdate
#define ShouldDisplayFavicon virtual ShouldDisplayFavicon
#define TryToCloseWindow virtual TryToCloseWindow
#define OnTabClosing virtual OnTabClosing
#define TabStripEmpty virtual TabStripEmpty
#define ResetTryToCloseWindow virtual ResetTryToCloseWindow
#define FullscreenControllerInteractiveTest \
  FullscreenControllerInteractiveTest;      \
  friend class BookmarkPrefsService;        \
  friend class BraveBrowser

#include "src/chrome/browser/ui/browser.h"  // IWYU pragma: export

#undef FullscreenControllerInteractiveTest
#undef ResetTryToCloseWindow
#undef TryToCloseWindow
#undef TabStripEmpty
#undef OnTabClosing
#undef ShouldDisplayFavicon
#undef ScheduleUIUpdate
#undef FinishWarnBeforeClosing

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_BROWSER_H_
