/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_H_

class BraveTabTest;
class BraveTab;

#define kMinimumContentsWidthForCloseButtons \
  kMinimumContentsWidthForCloseButtons = 55; \
  friend class ::BraveTabTest;               \
  friend class ::BraveTab;                   \
  static constexpr int kMinimumContentsWidthForCloseButtons_UnUsed

#define GetWidthOfLargestSelectableRegion \
  virtual GetWidthOfLargestSelectableRegion

#define SetData virtual SetData
#define ActiveStateChanged virtual ActiveStateChanged
#define GetGroupColor virtual GetGroupColor
#define UpdateIconVisibility virtual UpdateIconVisibility
#define ShouldRenderAsNormalTab virtual ShouldRenderAsNormalTab
#define MaybeAdjustLeftForPinnedTab virtual MaybeAdjustLeftForPinnedTab

#include "src/chrome/browser/ui/views/tabs/tab.h"  // IWYU pragma: export

#undef MaybeAdjustLeftForPinnedTab
#undef ShouldRenderAsNormalTab
#undef UpdateIconVisibility
#undef GetGroupColor
#undef ActiveStateChanged
#undef GetWidthOfLargestSelectableRegion
#undef kMinimumContentsWidthForCloseButtons
#undef SetData

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_H_
