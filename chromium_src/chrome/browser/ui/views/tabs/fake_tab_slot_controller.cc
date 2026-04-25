/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/tabs/public/tree_tab_node.h"

#include <chrome/browser/ui/views/tabs/fake_tab_slot_controller.cc>

bool FakeTabSlotController::ShouldAlwaysHideCloseButton() const {
  return should_always_hide_close_button_;
}

bool FakeTabSlotController::CanCloseTabViaMiddleButtonClick() const {
  return can_close_tab_via_middle_button_click_;
}

bool FakeTabSlotController::IsVerticalTabsFloating() const {
  return false;
}

bool FakeTabSlotController::IsVerticalTabsAnimatingButNotFinalState() const {
  return false;
}

bool FakeTabSlotController::ShouldPaintTabAccent(const Tab* tab) const {
  return false;
}

std::optional<TabAccentColors> FakeTabSlotController::GetTabAccentColors(
    const Tab* tab) const {
  return std::nullopt;
}

ui::ImageModel FakeTabSlotController::GetTabAccentIcon(const Tab* tab) const {
  return ui::ImageModel();
}

int FakeTabSlotController::GetTreeHeight(
    const tree_tab::TreeTabNodeId& id) const {
  return 0;
}

const tabs::TreeTabNode* FakeTabSlotController::GetTreeTabNode(
    const tree_tab::TreeTabNodeId& id) const {
  return nullptr;
}

void FakeTabSlotController::SetTreeTabNodeCollapsed(
    const tree_tab::TreeTabNodeId& id,
    bool collapsed) {}

bool FakeTabSlotController::IsInCollapsedTreeTabNode(
    const tree_tab::TreeTabNodeId& id) const {
  return false;
}

brave_tabs::TabMinWidthMode FakeTabSlotController::GetTabMinWidthMode() const {
  return tab_min_width_mode_;
}
