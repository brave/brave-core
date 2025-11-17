/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_SLOT_CONTROLLER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_SLOT_CONTROLLER_H_

#define IsGroupCollapsed(...)                                  \
  IsGroupCollapsed(__VA_ARGS__) const = 0;                     \
  virtual void SetCustomTitleForTab(                           \
      Tab* tab, const std::optional<std::u16string>& title) {} \
  virtual const Browser* GetBrowser()

// Add a method to TabSlotController to determine whether to hide the close
// button regardless of its state.
#define ShouldCompactLeadingEdge()      \
  ShouldCompactLeadingEdge() const = 0; \
  virtual bool ShouldAlwaysHideCloseButton()

#include <chrome/browser/ui/views/tabs/tab_slot_controller.h>  // IWYU pragma: export

#undef ShouldCompactLeadingEdge
#undef IsGroupCollapsed

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_SLOT_CONTROLLER_H_
