/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_SLOT_CONTROLLER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_SLOT_CONTROLLER_H_

#define ShiftGroupLeft(...)                                                    \
  SetCustomTitleForTab(Tab* tab, const std::optional<std::u16string>& title) { \
  }                                                                            \
  virtual void ShiftGroupLeft(__VA_ARGS__)

// Add a method to TabSlotController to determine whether to hide the close
// button regardless of its state.
#define ShouldCompactLeadingEdge()      \
  ShouldCompactLeadingEdge() const = 0; \
  virtual bool ShouldAlwaysHideCloseButton()

// Add a method to TabSlotController to check if vertical tabs are in floating
// mode.
#define EndDrag(...)        \
  EndDrag(__VA_ARGS__) = 0; \
  virtual bool IsVerticalTabsFloating() const

// Add a method to TabSlotController to determine whether tabs can be closed via
// middle mouse button click.
#define CanPaintThrobberToLayer()      \
  CanPaintThrobberToLayer() const = 0; \
  virtual bool CanCloseTabViaMiddleButtonClick()

#include <chrome/browser/ui/views/tabs/tab_slot_controller.h>  // IWYU pragma: export

#undef CanPaintThrobberToLayer
#undef EndDrag
#undef ShouldCompactLeadingEdge
#undef ShiftGroupLeft

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_SLOT_CONTROLLER_H_
