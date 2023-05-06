/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/brave_contents_layout_manager.h"

#include <algorithm>

#include "ui/views/view.h"

BraveContentsLayoutManager::BraveContentsLayoutManager(
    views::View* devtools_view,
    views::View* contents_view,
    views::View* sidebar_container_view)
    : ContentsLayoutManager(devtools_view, contents_view),
      sidebar_container_view_(sidebar_container_view) {
  DCHECK(sidebar_container_view_);
}

BraveContentsLayoutManager::~BraveContentsLayoutManager() = default;

void BraveContentsLayoutManager::Layout(views::View* contents_container) {
  DCHECK(host_ == contents_container);

  // Use upstream layout logic when sidebar is not shown.
  if (!sidebar_container_view_->GetVisible()) {
    return ContentsLayoutManager::Layout(contents_container);
  }

  int proposed_sidebar_width =
      sidebar_container_view_->GetPreferredSize().width();
  int contents_width = contents_container->width();

  // Guarantee 20% width for contents at least.
  if ((contents_width - proposed_sidebar_width) <= contents_width * 0.2) {
    proposed_sidebar_width = std::min(static_cast<int>(contents_width * 0.8),
                                      proposed_sidebar_width);
  }
  contents_width -= proposed_sidebar_width;

  int sidebar_x = 0;
  if (!sidebar_on_left_) {
    sidebar_x =
        contents_container->GetLocalBounds().right() - proposed_sidebar_width;
  }

  const int contents_height = contents_container->height();
  const gfx::Rect bounds(sidebar_x, 0, proposed_sidebar_width, contents_height);
  sidebar_container_view_->SetBoundsRect(host_->GetMirroredRect(bounds));

  if (reader_mode_panel_view_ && reader_mode_panel_view_->GetVisible()) {
    contents_height -= reader_mode_panel_view_->GetPreferredSize().height();
  }
  gfx::Size container_size(contents_width, contents_height);
  gfx::Rect new_devtools_bounds;
  gfx::Rect new_contents_bounds;
  gfx::Rect reader_mode_panel_bounds;

  ApplyDevToolsContentsResizingStrategy(
      strategy_, container_size, &new_devtools_bounds, &new_contents_bounds);

  new_devtools_bounds.Offset(sidebar_on_left_ ? proposed_sidebar_width : 0, 0);
  new_contents_bounds.Offset(sidebar_on_left_ ? proposed_sidebar_width : 0, 0);

  if (reader_mode_panel_view_ && reader_mode_panel_view_->GetVisible()) {
    reader_mode_panel_bounds.SetRect(
        new_contents_bounds.x(), 0, new_contents_bounds.width(),
        reader_mode_panel_view_->GetPreferredSize().height());
    new_contents_bounds.set_y(reader_mode_panel_bounds.height());
  }
  // DevTools cares about the specific position, so we have to compensate RTL
  // layout here.
  devtools_view_->SetBoundsRect(host_->GetMirroredRect(new_devtools_bounds));
  contents_view_->SetBoundsRect(host_->GetMirroredRect(new_contents_bounds));

  if (reader_mode_panel_view_) {
    reader_mode_panel_view_->SetBoundsRect(
        host_->GetMirroredRect(reader_mode_panel_bounds));
  }
}
