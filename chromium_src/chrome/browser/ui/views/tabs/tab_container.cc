/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_tab_container.h"
#include "brave/browser/ui/views/tabs/brave_vertical_tab_utils.h"

#include "src/chrome/browser/ui/views/tabs/tab_container.cc"

// In order to instantiate RemoveTabDelegate which is defined in
// tab_container.cc, this method is defined here.
void BraveTabContainer::StartRemoveTabAnimation(Tab* tab,
                                                int former_model_index) {
  if (!tabs::ShouldShowVerticalTabs()) {
    TabContainer::StartRemoveTabAnimation(tab, former_model_index);
    return;
  }

  if (in_tab_close_ && GetTabCount() > 0 &&
      override_available_width_for_tabs_ >
          tabs_view_model_.ideal_bounds(GetTabCount() - 1).right()) {
    // Tab closing mode is no longer constraining tab widths - they're at full
    // size. Exit tab closing mode so that it doesn't artificially inflate our
    // bounds.
    ExitTabClosingMode();
  }

  StartBasicAnimation();

  const int tab_overlap = TabStyle::GetTabOverlap();

  // TODO(pkasting): When closing multiple tabs, we get repeated RemoveTabAt()
  // calls, each of which closes a new tab and thus generates different ideal
  // bounds.  We should update the animations of any other tabs that are
  // currently being closed to reflect the new ideal bounds, or else change from
  // removing one tab at a time to animating the removal of all tabs at once.

  // Compute the target bounds for animating this tab closed.  The tab's left
  // edge should stay joined to the right edge of the previous tab, if any.
  gfx::Rect target_bounds = tab->bounds();
  target_bounds.set_y(
      (former_model_index > 0)
          ? tabs_view_model_.ideal_bounds(former_model_index - 1).bottom()
          : 0);

  // The tab should animate to the width of the overlap in order to close at the
  // same speed the surrounding tabs are moving, since at this width the
  // subsequent tab is naturally positioned at the same X coordinate.
  target_bounds.set_width(tab_overlap);
  bounds_animator().AnimateViewTo(
      tab, target_bounds,
      std::make_unique<TabContainer::RemoveTabDelegate>(
          this, tab,
          base::BindRepeating(&TabContainer::OnTabSlotAnimationProgressed,
                              base::Unretained(this))));
}
