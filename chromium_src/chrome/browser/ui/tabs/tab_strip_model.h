/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TABS_TAB_STRIP_MODEL_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TABS_TAB_STRIP_MODEL_H_

// Brave-specific tab strip model commands:
//   CommandRestoreTab - Restores a recently closed tab from history
//   CommandBookmarkAllTabs - Bookmarks all tabs in the current window
//   CommandShowVerticalTabs - Toggles vertical tab strip display mode
//   CommandToggleTabMuted - Toggles mute/unmute state of selected tabs
//   CommandBringAllTabsToThisWindow
//     - Moves all tabs from other windows to the current window
//   CommandCloseDuplicateTabs - Closes duplicate tabs with the same URL
//   CommandOpenInContainer - Container submenu
//   CommandRenameTab - Allows renaming a tab with custom title
#define CommandLast                                                        \
  CommandRestoreTab, CommandBookmarkAllTabs, CommandShowVerticalTabs,      \
      CommandToggleTabMuted, CommandBringAllTabsToThisWindow,              \
      CommandCloseDuplicateTabs, CommandOpenInContainer, CommandRenameTab, \
      CommandLast

#define SelectRelativeTab(...)            \
  virtual SelectRelativeTab(__VA_ARGS__); \
  friend class BraveTabStripModel

#define DraggingTabsSession DraggingTabsSessionChromium
#define IsReadLaterSupportedForAny virtual IsReadLaterSupportedForAny

#include <chrome/browser/ui/tabs/tab_strip_model.h>  // IWYU pragma: export

#undef IsReadLaterSupportedForAny
#undef DraggingTabsSession
#undef SelectRelativeTab
#undef CommandLast

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TABS_TAB_STRIP_MODEL_H_
