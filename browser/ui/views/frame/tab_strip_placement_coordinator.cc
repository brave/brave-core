/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/tab_strip_placement_coordinator.h"

#include "base/check_deref.h"
#include "brave/browser/ui/focus_mode/focus_mode_utils.h"
#include "brave/browser/ui/views/tabs/vertical_tab_controller.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "ui/views/view.h"

TabStripPlacementCoordinator::TabStripPlacementCoordinator(
    base::PassKey<BraveBrowserView>,
    BrowserWindowInterface* browser_window_interface,
    views::View* tab_strip_region_view)
    : browser_window_interface_(CHECK_DEREF(browser_window_interface)),
      tab_strip_region_view_(CHECK_DEREF(tab_strip_region_view)) {
  auto* original_parent = tab_strip_region_view_->parent();
  CHECK(original_parent);

  auto original_index =
      original_parent->GetIndexOf(base::to_address(tab_strip_region_view_));

  SetPlacement(PlacementKind::kDefault, original_parent, original_index);
}

TabStripPlacementCoordinator::~TabStripPlacementCoordinator() {
  // On destruction, restore the tabstrip to its original placement so that
  // browser view teardown runs as expected. Clear all placements other than the
  // default placement and trigger an update.
  for (auto& [kind, placement] : placements_) {
    if (kind != PlacementKind::kDefault) {
      placement = {};
    }
  }
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
  auto* browser = base::to_address(browser_window_interface_);
  auto* vertical_tab_controller =
      browser ? browser->GetFeatures().vertical_tab_controller() : nullptr;

  auto get_placement = [&]() -> const Placement& {
    if (vertical_tab_controller &&
        vertical_tab_controller->ShouldShowBraveVerticalTabs()) {
      auto& placement = placements_[PlacementKind::kVerticalTabStrip];
      if (placement.parent) {
        return placement;
      }
    }
    if (IsFocusModeEnabled(browser)) {
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
    // The following remove-then-add sequence is required in order to trigger
    // AddedToWidget in BraveTabStrip, which calls SetAvailableWidthCallback as
    // appropriate for the current tab strip orientation.
    if (tab_strip_region_view_->parent()) {
      tab_strip_region_view_->parent()->RemoveChildView(
          base::to_address(tab_strip_region_view_));
    }
    size_t max_index = parent->children().size();
    size_t add_at = std::min(placement.index.value_or(max_index), max_index);
    parent->AddChildViewAt(base::to_address(tab_strip_region_view_), add_at);
  }
}
