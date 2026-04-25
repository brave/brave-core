/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_STRIP_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_STRIP_H_

#include "brave/browser/ui/views/tabs/accent_color/brave_tab_accent_types.h"
#include "brave/browser/ui/views/tabs/brave_tab_container.h"
#include "chrome/browser/ui/views/tabs/tab_container.h"
#include "chrome/browser/ui/views/tabs/tab_slot_controller.h"

class BraveTabHoverCardController;

#define UpdateHoverCard                         \
  UpdateHoverCard_Unused();                     \
  friend class BraveTabDragContext;             \
  friend class BraveTabHoverTest;               \
  friend class BraveTabStrip;                   \
  friend class BraveVerticalTabStripRegionView; \
  void UpdateHoverCard

#define GetDragContext                                                       \
  Unused_GetDragContext() {                                                  \
    return nullptr;                                                          \
  }                                                                          \
  bool ShouldAlwaysHideCloseButton() const override;                         \
  bool CanCloseTabViaMiddleButtonClick() const override;                     \
  bool IsVerticalTabsFloating() const override;                              \
  bool IsVerticalTabsAnimatingButNotFinalState() const override;             \
  bool ShouldPaintTabAccent(const Tab* tab) const override;                  \
  std::optional<TabAccentColors> GetTabAccentColors(const Tab* tab)          \
      const override;                                                        \
  ui::ImageModel GetTabAccentIcon(const Tab* tab) const override;            \
  int GetTreeHeight(const tree_tab::TreeTabNodeId& id) const override;       \
  const tabs::TreeTabNode* GetTreeTabNode(const tree_tab::TreeTabNodeId& id) \
      const override;                                                        \
  void SetTreeTabNodeCollapsed(const tree_tab::TreeTabNodeId& id,            \
                               bool collapsed) override {}                   \
  bool IsInCollapsedTreeTabNode(const tree_tab::TreeTabNodeId& id)           \
      const override;                                                        \
  brave_tabs::TabMinWidthMode GetTabMinWidthMode() const override;           \
  virtual TabDragContext* GetDragContext

#define TabHoverCardController BraveTabHoverCardController
#define AddTabToGroup virtual AddTabToGroup
#define SetTabData virtual SetTabData
#define MoveTab virtual MoveTab
#include <chrome/browser/ui/views/tabs/tab_strip.h>  // IWYU pragma: export
#undef MoveTab
#undef SetTabData
#undef AddTabToGroup
#undef TabHoverCardController
#undef GetDragContext
#undef UpdateHoverCard

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_STRIP_H_
