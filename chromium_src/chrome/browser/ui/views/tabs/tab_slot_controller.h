/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_SLOT_CONTROLLER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_SLOT_CONTROLLER_H_

#include "brave/browser/ui/views/tabs/accent_color/brave_tab_accent_types.h"
#include "ui/base/models/image_model.h"

namespace tabs {
class TreeTabNode;
}  // namespace tabs

namespace tree_tab {
class TreeTabNodeId;
}  // namespace tree_tab

// Add a method to TabSlotController to determine whether to hide the close
// button regardless of its state.
#define ShouldCompactLeadingEdge()      \
  ShouldCompactLeadingEdge() const = 0; \
  virtual bool ShouldAlwaysHideCloseButton()

// Add a method to TabSlotController to check if vertical tabs are in floating
// mode.
#define EndDrag(...)                               \
  EndDrag(__VA_ARGS__) = 0;                        \
  virtual bool IsVerticalTabsFloating() const = 0; \
  virtual bool IsVerticalTabsAnimatingButNotFinalState() const

// Add methods to TabSlotController to determine whether the tab should have an
// accent painted.
#define CanPaintThrobberToLayer()                                              \
  /* Returns whether the tab should have an accent painted (e.g., container */ \
  /* tab, or other features that require visual accent). This allows the */    \
  /* tab style to determine if accent painting is needed without exposing */   \
  /* implementation details. */                                                \
  ShouldPaintTabAccent(const Tab* tab) const = 0;                              \
  /* Returns the accent colors (border, background) for the tab if it */       \
  /* should have an accent painted. Returns nullopt if the tab should not */   \
  /* have an accent or colors cannot be determined. */                         \
  virtual std::optional<TabAccentColors> GetTabAccentColors(const Tab* tab)    \
      const = 0;                                                               \
  /* Returns the accent icon for the tab if it should have an accent */        \
  /* painted. Returns an empty ImageModel if the tab should not have an */     \
  /* accent or icon cannot be determined. */                                   \
  virtual ui::ImageModel GetTabAccentIcon(const Tab* tab) const = 0;           \
  /* Method to TabSlotController to determine whether tabs can be closed */    \
  /* via middle mouse button click. */                                         \
  virtual bool CanCloseTabViaMiddleButtonClick() const = 0;                    \
  virtual bool CanPaintThrobberToLayer()

// Add methods for Tree Tab operations to TabSlotController
#define ShiftGroupRight(...)                                              \
  ShiftGroupRight(__VA_ARGS__) = 0;                                       \
  virtual int GetTreeHeight(const tree_tab::TreeTabNodeId& id) const = 0; \
  virtual const tabs::TreeTabNode& GetTreeTabNode(                        \
      const tree_tab::TreeTabNodeId& id) const;                           \
  virtual void SetTreeTabNodeCollapsed(const tree_tab::TreeTabNodeId& id, \
                                       bool collapsed) = 0;               \
  virtual bool IsInCollapsedTreeTabNode(const tree_tab::TreeTabNodeId& id) const

#include <chrome/browser/ui/views/tabs/tab_slot_controller.h>  // IWYU pragma: export

#undef ShiftGroupRight
#undef CanPaintThrobberToLayer
#undef EndDrag
#undef ShouldCompactLeadingEdge
#undef ShiftGroupLeft

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_SLOT_CONTROLLER_H_
