/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/tab_strip_placement_coordinator.h"

#include "brave/browser/ui/focus_mode/focus_mode_utils.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "ui/views/view.h"

TabStripPlacementCoordinator::TabStripPlacementCoordinator(
    BrowserWindowInterface* browser_window_interface,
    views::View* tab_strip_region_view)
    : browser_window_interface_(browser_window_interface),
      tab_strip_region_view_(tab_strip_region_view) {
  CHECK(browser_window_interface_);
  CHECK(tab_strip_region_view_);

  auto* original_parent = tab_strip_region_view_->parent();
  CHECK(original_parent);

  SetPlacement(PlacementKind::kDefault, original_parent,
               original_parent->GetIndexOf(tab_strip_region_view_));
}

TabStripPlacementCoordinator::~TabStripPlacementCoordinator() = default;

void TabStripPlacementCoordinator::SetEnabled(bool enabled) {
  if (enabled == enabled_) {
    return;
  }
  enabled_ = enabled;
  UpdatePlacement();
}

void TabStripPlacementCoordinator::SetPlacement(PlacementKind kind,
                                                views::View* parent,
                                                std::optional<size_t> index) {
  placements_[kind] = {.parent = parent, .index = index};
}

void TabStripPlacementCoordinator::ClearPlacement(PlacementKind kind) {
  placements_[kind] = {};
}

void TabStripPlacementCoordinator::UpdatePlacement() {
  if (!enabled_) {
    return;
  }

  auto get_placement = [&]() -> const Placement& {
    if (tabs::utils::ShouldShowBraveVerticalTabs(browser_window_interface_)) {
      auto& placement = placements_[PlacementKind::kVerticalTabStrip];
      if (placement.parent) {
        return placement;
      }
    }
    if (IsFocusModeEnabled(browser_window_interface_)) {
      auto& placement = placements_[PlacementKind::kTopContainer];
      if (placement.parent) {
        return placement;
      }
    }
    return placements_[PlacementKind::kDefault];
  };

  auto placement = get_placement();
  auto* parent = placement.parent.get();

  if (parent && parent != tab_strip_region_view_->parent()) {
    // The following remove-then-add sequence is currently required in order to
    // trigger AddedToWidget in BraveTabStrip, which calls
    // SetAvailableWidthCallback as appropriate for the current tab strip
    // orientation.
    if (tab_strip_region_view_->parent()) {
      tab_strip_region_view_->parent()->RemoveChildView(
          tab_strip_region_view_.get());
    }
    parent->AddChildViewAt(tab_strip_region_view_.get(),
                           placement.index.value_or(parent->children().size()));
  }
}
