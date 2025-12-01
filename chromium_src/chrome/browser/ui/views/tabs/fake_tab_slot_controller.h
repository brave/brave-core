/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_FAKE_TAB_SLOT_CONTROLLER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_FAKE_TAB_SLOT_CONTROLLER_H_

#include "chrome/browser/ui/views/tabs/tab_slot_controller.h"

#define IsGroupCollapsed(...)                   \
  IsGroupCollapsed(__VA_ARGS__) const override; \
  const Browser* GetBrowser()

// Add override for ShouldAlwaysHideTabCloseButton() and setters to control its
// return value in tests.
#define ShouldCompactLeadingEdge()                      \
  ShouldCompactLeadingEdge() const override;            \
                                                        \
 private:                                               \
  bool should_always_hide_close_button_ = false;        \
                                                        \
 public:                                                \
  void set_should_always_hide_close_button(bool hide) { \
    should_always_hide_close_button_ = hide;            \
  }                                                     \
  bool ShouldAlwaysHideCloseButton()

// Add override for IsVerticalTabsFloating()
#define EndDrag(...)             \
  EndDrag(__VA_ARGS__) override; \
  bool IsVerticalTabsFloating() const

#include <chrome/browser/ui/views/tabs/fake_tab_slot_controller.h>  // IWYU pragma: export

#undef EndDrag
#undef ShouldCompactLeadingEdge
#undef IsGroupCollapsed

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_FAKE_TAB_SLOT_CONTROLLER_H_
