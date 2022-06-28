/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_H_

class BraveTabTest;

#define kMinimumContentsWidthForCloseButtons \
  kMinimumContentsWidthForCloseButtons = 55; \
  static constexpr int kMinimumContentsWidthForCloseButtons_UnUsed

#define UpdateIconVisibility     \
  UpdateIconVisibility_Unused(); \
  friend class ::BraveTabTest;   \
  void UpdateIconVisibility

#define GetWidthOfLargestSelectableRegion    \
  GetWidthOfLargestSelectableRegion() const; \
  int GetWidthOfLargestSelectableRegion_ChromiumImpl
#define ActiveStateChanged           \
  ActiveStateChanged_ChromiumImpl(); \
  void ActiveStateChanged
#include "src/chrome/browser/ui/views/tabs/tab.h"
#undef ActiveStateChanged
#undef GetWidthOfLargestSelectableRegion
#undef kMinimumContentsWidthForCloseButtons
#undef UpdateIconVisibility

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_H_
