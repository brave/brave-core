/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_FAKE_TAB_SLOT_CONTROLLER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_FAKE_TAB_SLOT_CONTROLLER_H_

#include "chrome/browser/ui/views/tabs/tab_slot_controller.h"

// Add override for
// ShouldAlwaysHideTabCloseButton()/CanCloseTabViaMiddleButtonClick() and
// setters to control its return value in tests.
#define ShouldCompactLeadingEdge()                               \
  ShouldCompactLeadingEdge() const override;                     \
                                                                 \
 private:                                                        \
  bool should_always_hide_close_button_ = false;                 \
  bool can_close_tab_via_middle_button_click_ = true;            \
                                                                 \
 public:                                                         \
  void set_should_always_hide_close_button(bool hide) {          \
    should_always_hide_close_button_ = hide;                     \
  }                                                              \
  void set_can_close_tab_via_middle_button_click(bool enabled) { \
    can_close_tab_via_middle_button_click_ = enabled;            \
  }                                                              \
  bool ShouldAlwaysHideCloseButton()

// Add override for IsVerticalTabsFloating()
#define EndDrag(...)             \
  EndDrag(__VA_ARGS__) override; \
  bool IsVerticalTabsFloating() const

// Add override for CanCloseTabViaMiddleButtonClick()
#define CanPaintThrobberToLayer()           \
  CanPaintThrobberToLayer() const override; \
  bool CanCloseTabViaMiddleButtonClick()

// Add overrides for TabAccent related methods
#define IsFrameCondensed()                                                 \
  ShouldPaintTabAccent(const Tab* tab) const override;                     \
  std::optional<SkColor> GetTabAccentColor(const Tab* tab) const override; \
  ui::ImageModel GetTabAccentIcon(const Tab* tab) const override;          \
  bool IsFrameCondensed()

#include <chrome/browser/ui/views/tabs/fake_tab_slot_controller.h>  // IWYU pragma: export

#undef IsFrameCondensed
#undef CanPaintThrobberToLayer
#undef EndDrag
#undef ShouldCompactLeadingEdge

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_FAKE_TAB_SLOT_CONTROLLER_H_
