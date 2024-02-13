/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/sidebar/sidebar_item_drag_context.h"

#include <optional>

SidebarItemDragContext::SidebarItemDragContext() = default;

SidebarItemDragContext::~SidebarItemDragContext() = default;

// Drag indicator is not equal to target index always.
// If item is moving to higher index, target index is 1 smaller index than
// indicator index because source item's room is reduced.
size_t SidebarItemDragContext::GetTargetIndex() const {
  size_t index = drag_indicator_index_.value_or(0);
  return index > source_index_ ? index - 1 : index;
}

bool SidebarItemDragContext::ShouldMoveItem() const {
  if (!drag_indicator_index_)
    return false;

  return GetTargetIndex() != source_index_;
}

void SidebarItemDragContext::Reset() {
  source_index_ = std::nullopt;
  source_ = nullptr;
  drag_indicator_index_ = std::nullopt;
}

// static
bool SidebarItemDragContext::CanStartDrag(const gfx::Point press_pt,
                                          const gfx::Point p) {
  // Determine if the mouse has moved beyond a minimum elasticity distance in
  // any direction from the starting point.
  constexpr int kMinimumDragDistance = 10;
  int x_offset = abs(p.x() - press_pt.x());
  int y_offset = abs(p.y() - press_pt.y());
  return sqrt(pow(static_cast<float>(x_offset), 2) +
              pow(static_cast<float>(y_offset), 2)) > kMinimumDragDistance;
}
