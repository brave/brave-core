/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_STRIP_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_STRIP_H_

#include "brave/browser/ui/views/tabs/brave_tab_container.h"
#include "chrome/browser/ui/views/tabs/tab_container.h"
#include "chrome/browser/ui/views/tabs/tab_slot_controller.h"

class BraveTabHoverCardController;

#define UpdateHoverCard                    \
  UpdateHoverCard_Unused();                \
  friend class BraveTabHoverTest;          \
  friend class BraveTabStrip;              \
  friend class VerticalTabStripRegionView; \
  void UpdateHoverCard

#define ShouldDrawStrokes   \
  UnUsed() { return true; } \
  virtual bool ShouldDrawStrokes
#define GetDragContext                                                  \
  Unused_GetDragContext() {                                             \
    return nullptr;                                                     \
  }                                                                     \
  friend class BraveTabStrip;                                           \
  friend class BraveTabDragContext;                                     \
  bool IsTabTiled(const Tab* tab) const override;                       \
  bool IsFirstTabInTile(const Tab* tab) const override;                 \
  static constexpr bool IsUsingBraveTabHoverCardController() {          \
    return std::is_same_v<std::unique_ptr<BraveTabHoverCardController>, \
                          decltype(TabStrip::hover_card_controller_)>;  \
  }                                                                     \
  virtual TabDragContext* GetDragContext
#define TabHoverCardController BraveTabHoverCardController
#include "src/chrome/browser/ui/views/tabs/tab_strip.h"  // IWYU pragma: export
#undef TabHoverCardController
#undef GetDragContext
#undef ShouldDrawStrokes
#undef UpdateHoverCard

static_assert(TabStrip::IsUsingBraveTabHoverCardController(),
              "Should use BraveTabHoverCardController");

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_STRIP_H_
