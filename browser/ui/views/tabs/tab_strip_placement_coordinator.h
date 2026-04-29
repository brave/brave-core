/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TABS_TAB_STRIP_PLACEMENT_COORDINATOR_H_
#define BRAVE_BROWSER_UI_VIEWS_TABS_TAB_STRIP_PLACEMENT_COORDINATOR_H_

#include <optional>

#include "base/containers/flat_map.h"
#include "base/memory/raw_ptr.h"

class BrowserWindowInterface;

namespace views {
class View;
}  // namespace views

// Coordinates parenting of the tab strip view within the view hierarchy, which
// can depend upon various browser states (e.g. vertical tabs). Placement is
// be triggered with a call to `UpdatePlacement`.
class TabStripPlacementCoordinator {
 public:
  TabStripPlacementCoordinator(BrowserWindowInterface* browser_window_interface,
                               views::View* tab_strip_region_view);

  TabStripPlacementCoordinator(const TabStripPlacementCoordinator&) = delete;
  TabStripPlacementCoordinator& operator=(const TabStripPlacementCoordinator&) =
      delete;

  ~TabStripPlacementCoordinator();

  // Enables or disables the tabstrip placement behavior.
  void SetEnabled(bool enabled);

  enum class PlacementKind {
    kDefault = 0,
    kTopContainer = 1,
    kVerticalTabStrip = 2,
  };

  // Associates a placement kind with a parent and an optional child index.
  void SetPlacement(PlacementKind kind,
                    views::View* parent,
                    std::optional<size_t> index = {});

  // Clears the association between a placement kind and a parent.
  void ClearPlacement(PlacementKind kind);

  // Updates the tab strip placement for the current browser state, potentially
  // moving the view under a new parent.
  void UpdatePlacement();

 private:
  struct Placement {
    raw_ptr<views::View> parent = nullptr;
    std::optional<size_t> index;
  };

  bool enabled_ = false;
  raw_ptr<BrowserWindowInterface> browser_window_interface_;
  raw_ptr<views::View> tab_strip_region_view_;
  base::flat_map<PlacementKind, Placement> placements_;
};

using TabStripPlacementKind = TabStripPlacementCoordinator::PlacementKind;

#endif  // BRAVE_BROWSER_UI_VIEWS_TABS_TAB_STRIP_PLACEMENT_COORDINATOR_H_
