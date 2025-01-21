/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/brave_contents_layout_manager.h"

#include "ui/views/view.h"

BraveContentsLayoutManager::~BraveContentsLayoutManager() = default;

void BraveContentsLayoutManager::LayoutImpl() {
  const bool is_host_empty = !host_view()->width();
  if (is_host_empty) {
    // When minimizing window, this can happen
    return;
  }

  if (!contents_reader_mode_toolbar_ ||
      !contents_reader_mode_toolbar_->GetVisible()) {
    return ContentsLayoutManager::LayoutImpl();
  }

  LayoutContents(host_view()->GetLocalBounds(), contents_view_,
                 contents_reader_mode_toolbar_, devtools_view_, strategy_);
}

void BraveContentsLayoutManager::LayoutContents(
    const gfx::Rect& bounds,
    views::View* contents_view,
    views::View* reader_mode_toolbar,
    views::View* devtools_view,
    const DevToolsContentsResizingStrategy& strategy) {
  gfx::Rect new_contents_bounds;
  gfx::Rect new_devtools_bounds;
  ApplyDevToolsContentsResizingStrategy(
      strategy, bounds.size(), &new_devtools_bounds, &new_contents_bounds);
  new_contents_bounds.set_x(bounds.x() + new_contents_bounds.x());
  new_devtools_bounds.set_x(bounds.x() + new_devtools_bounds.x());

  if (reader_mode_toolbar && reader_mode_toolbar->GetVisible()) {
    gfx::Rect toolbar_bounds = new_contents_bounds;
    toolbar_bounds.set_height(reader_mode_toolbar->GetPreferredSize().height());
    reader_mode_toolbar->SetBoundsRect(toolbar_bounds);
    new_contents_bounds.Inset(
        gfx::Insets::TLBR(toolbar_bounds.height(), 0, 0, 0));
  }

  // TODO(sko) We're ignoring dev tools specific position. Maybe we need
  // to revisit this. On the other hand, I think we shouldn't let devtools
  // on the side of split view as it's too confusing.
  contents_view->SetBoundsRect(new_contents_bounds);
  devtools_view->SetBoundsRect(new_devtools_bounds);
}
