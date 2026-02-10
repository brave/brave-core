/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_SLOT_CONTROLLER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_SLOT_CONTROLLER_H_

#include "ui/base/models/image_model.h"

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

// Add methods to TabSlotController to determine whether the tab should have an
// accent painted.
#define IsFrameCondensed()                                                     \
  /* Returns whether the tab should have an accent painted (e.g., container */ \
  /* tab, or other features that require visual accent). This allows the */    \
  /* tab style to determine if accent painting is needed without exposing */   \
  /* implementation details. */                                                \
  ShouldPaintTabAccent(const Tab* tab) const = 0;                              \
  /* Returns the accent color for the tab if it should have an accent */       \
  /* painted. Returns nullopt if the tab should not have an accent or color */ \
  /* cannot be determined. */                                                  \
  virtual std::optional<SkColor> GetTabAccentColor(const Tab* tab) const = 0;  \
  /* Returns the accent icon for the tab if it should have an accent */        \
  /* painted. Returns an empty ImageModel if the tab should not have an */     \
  /* accent or icon cannot be determined. */                                   \
  virtual ui::ImageModel GetTabAccentIcon(const Tab* tab) const = 0;           \
  virtual bool IsFrameCondensed()

#include <chrome/browser/ui/views/tabs/tab_slot_controller.h>  // IWYU pragma: export

#undef IsFrameCondensed
#undef CanPaintThrobberToLayer
#undef EndDrag
#undef ShouldCompactLeadingEdge
#undef ShiftGroupLeft

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_SLOT_CONTROLLER_H_
