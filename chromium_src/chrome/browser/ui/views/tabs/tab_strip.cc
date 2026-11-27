/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/tabs/tab_strip.h"

#include <cmath>
#include <optional>
#include <vector>

#include "brave/browser/ui/tabs/public/vertical_tab_controller.h"
#include "brave/browser/ui/views/tabs/brave_tab.h"
#include "brave/browser/ui/views/tabs/brave_tab_container.h"
#include "brave/browser/ui/views/tabs/brave_tab_group_header.h"
#include "brave/browser/ui/views/tabs/brave_tab_hover_card_controller.h"
#include "brave/browser/ui/views/tabs/brave_tab_strip_layout_helper.h"
#include "brave/browser/ui/views/tabs/dragging/tab_drag_controller.h"
#include "chrome/browser/ui/ui_features.h"
#include "chrome/browser/ui/views/tabs/browser_tab_strip_controller.h"
#include "chrome/browser/ui/views/tabs/dragging/tab_drag_context.h"
#include "chrome/browser/ui/views/tabs/dragging/tab_drag_controller.h"
#include "chrome/browser/ui/views/tabs/tab_container.h"

#if BUILDFLAG(IS_WIN)
#include "ui/gfx/win/hwnd_util.h"
#endif

namespace tabs {
namespace {

// Returns bounds only when vertical tabs are shown. Otherwise returns
// std::nullopt and the caller should proceed with the upstream logic.
std::optional<std::vector<gfx::Rect>>
MaybeCalculateBoundsForVerticalDraggedViews(
    const std::vector<views::View*>& views,
    TabStrip* tab_strip) {
  auto* vertical_tab_controller =
      VerticalTabController::From(tab_strip->GetBrowserWindowInterface());
  if (!vertical_tab_controller ||
      !vertical_tab_controller->ShouldShowBraveVerticalTabs()) {
    return std::nullopt;
  }
  return CalculateBoundsForVerticalDraggedViews(views, tab_strip);
}

// Returns true if vertical tabs are shown and the candidate was handled, in
// which case the caller should skip the upstream logic for this candidate.
bool MaybeUpdateInsertionIndexForVerticalTabs(
    const gfx::Rect& dragged_bounds,
    int first_dragged_tab_index,
    int num_dragged_tabs,
    bool dragged_group,
    int candidate_index,
    TabStripController* tab_strip_controller,
    TabContainer* tab_container,
    int& min_distance,
    int& min_distance_index,
    TabStrip* tab_strip) {
  auto* vertical_tab_controller =
      VerticalTabController::From(tab_strip->GetBrowserWindowInterface());
  if (!vertical_tab_controller ||
      !vertical_tab_controller->ShouldShowBraveVerticalTabs()) {
    return false;
  }

  UpdateInsertionIndexForVerticalTabs(
      dragged_bounds, first_dragged_tab_index, num_dragged_tabs, dragged_group,
      candidate_index, tab_strip_controller, tab_container, min_distance,
      min_distance_index, tab_strip);
  return true;
}

}  // namespace
}  // namespace tabs

#include <chrome/browser/ui/views/tabs/tab_strip.cc>

bool TabStrip::ShouldAlwaysHideCloseButton() const {
  return false;
}

bool TabStrip::CanCloseTabViaMiddleButtonClick() const {
  return true;
}

bool TabStrip::IsVerticalTabsFloating() const {
  return false;
}

bool TabStrip::IsVerticalTabsAnimatingButNotFinalState() const {
  return false;
}

bool TabStrip::ShouldPaintTabAccent(const Tab* tab) const {
  return false;
}

std::optional<TabAccentColors> TabStrip::GetTabAccentColors(
    const Tab* tab) const {
  return std::nullopt;
}

ui::ImageModel TabStrip::GetTabAccentIcon(const Tab* tab) const {
  return ui::ImageModel();
}

bool TabStrip::ShouldAlwaysShowMiniTabAccent() const {
  return false;
}

int TabStrip::GetTreeHeight(const tree_tab::TreeTabNodeId& id) const {
  return 0;
}

const tabs::TreeTabNode* TabStrip::GetTreeTabNode(
    const tree_tab::TreeTabNodeId& id) const {
  return nullptr;
}

bool TabStrip::IsInCollapsedTreeTabNode(
    const tree_tab::TreeTabNodeId& id) const {
  return false;
}

brave_tabs::TabMinWidthMode TabStrip::GetTabMinWidthMode() const {
  return {};
}

bool TabStrip::IsHorizontalScrollingEnabled() const {
  return false;
}
