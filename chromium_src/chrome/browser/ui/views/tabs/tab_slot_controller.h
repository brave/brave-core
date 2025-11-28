/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_SLOT_CONTROLLER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_SLOT_CONTROLLER_H_

#include "brave/components/tabs/public/tree_tab_node.h"
#include "brave/components/tabs/public/tree_tab_node_id.h"

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

// Add interface methods to TreeTabNode with its id.
#define ShiftGroupRight(...)                                               \
  ShiftGroupRight(__VA_ARGS__) = 0;                                        \
  /* returns the height of the tree that given TreeTabNodeId belongs to */ \
  virtual int GetTreeHeight(const tree_tab::TreeTabNodeId& id) const = 0;  \
  virtual const tabs::TreeTabNode& GetTreeTabNode(                         \
      const tree_tab::TreeTabNodeId& id) const

#include <chrome/browser/ui/views/tabs/tab_slot_controller.h>  // IWYU pragma: export

#undef ShiftGroupRight
#undef ShouldCompactLeadingEdge
#undef IsGroupCollapsed

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_SLOT_CONTROLLER_H_
