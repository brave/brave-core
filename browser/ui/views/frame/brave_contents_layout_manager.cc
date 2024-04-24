/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/brave_contents_layout_manager.h"

#include "base/feature_list.h"
#include "brave/browser/ui/tabs/features.h"
#include "ui/views/view.h"

BraveContentsLayoutManager::~BraveContentsLayoutManager() = default;

void BraveContentsLayoutManager::SetSecondaryContentsResizingStrategy(
    const DevToolsContentsResizingStrategy& strategy) {
  if (secondary_strategy_.Equals(strategy)) {
    return;
  }

  secondary_strategy_.CopyFrom(strategy);
  if (host_view()) {
    host_view()->InvalidateLayout();
  }
}

void BraveContentsLayoutManager::LayoutImpl() {
  if (!base::FeatureList::IsEnabled(tabs::features::kBraveSplitView) ||
      !secondary_contents_view_ || !secondary_devtools_view_ ||
      !secondary_contents_view_->GetVisible()) {
    ContentsLayoutManager::LayoutImpl();
    return;
  }

  auto layout_web_contents_and_devtools =
      [](gfx::Rect bounds, views::View* contents_view,
         views::View* devtools_view,
         const DevToolsContentsResizingStrategy& strategy) {
        gfx::Rect new_contents_bounds;
        gfx::Rect new_devtools_bounds;
        ApplyDevToolsContentsResizingStrategy(strategy, bounds.size(),
                                              &new_devtools_bounds,
                                              &new_contents_bounds);
        new_contents_bounds.set_x(bounds.x() + new_contents_bounds.x());
        new_devtools_bounds.set_x(bounds.x() + new_devtools_bounds.x());

        // TODO(sko) We're ignoring dev tools specific position. Maybe we need
        // to revisit this. On the other hand, I think we shouldn't let devtools
        // on the side of split view as it's too confusing.
        contents_view->SetBoundsRect(new_contents_bounds);
        devtools_view->SetBoundsRect(new_devtools_bounds);
      };

  gfx::Rect bounds = host_view()->GetLocalBounds();
  bounds.set_width((bounds.width() - kSpacingBetweenContentsWebViews) / 2);
  if (show_main_web_contents_at_tail_) {
    layout_web_contents_and_devtools(bounds, secondary_contents_view_,
                                     secondary_devtools_view_,
                                     secondary_strategy_);
  } else {
    layout_web_contents_and_devtools(bounds, contents_view_, devtools_view_,
                                     strategy_);
  }

  // In case of odd width, give the remaining
  // width to the secondary contents view.
  bounds.set_x(bounds.width() + kSpacingBetweenContentsWebViews);
  bounds.set_width(host_view()->width() - bounds.width() -
                   kSpacingBetweenContentsWebViews);
  if (show_main_web_contents_at_tail_) {
    layout_web_contents_and_devtools(bounds, contents_view_, devtools_view_,
                                     strategy_);
  } else {
    layout_web_contents_and_devtools(bounds, secondary_contents_view_,
                                     secondary_devtools_view_,
                                     secondary_strategy_);
  }
}
