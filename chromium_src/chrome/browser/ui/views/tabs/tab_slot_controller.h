/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_SLOT_CONTROLLER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_SLOT_CONTROLLER_H_

#define IsGroupCollapsed                                       \
  IsTabTiled(const Tab* tab) const = 0;                        \
  virtual bool IsFirstTabInTile(const Tab* tab) const = 0;     \
  virtual const Browser* GetBrowser() const = 0;               \
  virtual void SetCustomTitleForTab(                           \
      Tab* tab, const std::optional<std::u16string>& title) {} \
  virtual bool IsGroupCollapsed

// Add interface methods to get level and height for Tree tabs
#define ShiftGroupRight(...)                           \
  ShiftGroupRight(__VA_ARGS__) = 0;                    \
  virtual int GetTreeHeight(const Tab* tab) const = 0; \
  virtual int GetTreeNodeLevel(const Tab* tab) const

#include <chrome/browser/ui/views/tabs/tab_slot_controller.h>  // IWYU pragma: export

#undef ShiftGroupRight
#undef IsGroupCollapsed

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_SLOT_CONTROLLER_H_
