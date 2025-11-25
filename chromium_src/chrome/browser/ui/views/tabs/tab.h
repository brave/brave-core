/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_H_

#include "base/memory/raw_ref.h"
#include "chrome/browser/ui/views/tabs/tab_slot_controller.h"

class BraveTabTest;
class BraveTab;
class Tab;

// A struct to wrap the state of whether the close button is shown on a tab,
// taking into account whether the controller wants to always hide close
// buttons.
struct ControllableCloseButtonState final {
  ControllableCloseButtonState(TabSlotController& controller, Tab& tab);
  ~ControllableCloseButtonState();

  bool operator=(bool show);

  // Disable "explicit" enforcement to allow implicit conversion to bool.
  // NOLINTNEXTLINE(google-explicit-constructor)
  operator bool() const;

  // Controller of the tab that owns this state.
  raw_ref<TabSlotController> controller;

  // Owner of this state.
  raw_ref<Tab> tab;

  bool showing_close_button = false;
};

#define kMinimumContentsWidthForCloseButtons      \
  kMinimumContentsWidthForCloseButtons = 55;      \
  bool IsTabMuteIndicatorNotClickable() override; \
  friend class ::BraveTabTest;                    \
  friend class ::BraveTab;                        \
  static constexpr int kMinimumContentsWidthForCloseButtons_UnUsed

#define GetWidthOfLargestSelectableRegion \
  virtual GetWidthOfLargestSelectableRegion

#define SetData virtual SetData
#define ActiveStateChanged virtual ActiveStateChanged
#define GetGroupColor virtual GetGroupColor
#define UpdateIconVisibility virtual UpdateIconVisibility
#define ShouldRenderAsNormalTab virtual ShouldRenderAsNormalTab
#define MaybeAdjustLeftForPinnedTab virtual MaybeAdjustLeftForPinnedTab
#define IsActive virtual IsActive

#include <chrome/browser/ui/views/tabs/tab.h>  // IWYU pragma: export

#undef IsActive
#undef MaybeAdjustLeftForPinnedTab
#undef ShouldRenderAsNormalTab
#undef UpdateIconVisibility
#undef GetGroupColor
#undef ActiveStateChanged
#undef GetWidthOfLargestSelectableRegion
#undef kMinimumContentsWidthForCloseButtons
#undef SetData

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_H_
