/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/brave_contents_layout_manager.h"

#include <vector>

#include "ui/views/view.h"

BraveContentsLayoutManager::BraveContentsLayoutManager(
    views::View* devtools_view,
    views::View* contents_view,
    views::View* sidebar_container_view,
    views::View* vertical_tabs_container)
    : ContentsLayoutManager(devtools_view, contents_view),
      sidebar_container_view_(sidebar_container_view),
      vertical_tabs_container_(vertical_tabs_container) {
  DCHECK(sidebar_container_view_ || vertical_tabs_container_);
}

BraveContentsLayoutManager::~BraveContentsLayoutManager() = default;

void BraveContentsLayoutManager::Layout(views::View* contents_container) {
  DCHECK(host_ == contents_container);

  int contents_height = contents_container->height();
  int contents_width = contents_container->width();

  std::vector<views::View*> left_side_candidate_views;
  std::vector<views::View*> right_side_candidate_views;
  if (sidebar_on_left_) {
    left_side_candidate_views.push_back(sidebar_container_view_);
  } else {
    right_side_candidate_views.push_back(sidebar_container_view_);
  }
  left_side_candidate_views.push_back(vertical_tabs_container_);

  int taken_left_width = 0;
  for (auto* view : left_side_candidate_views) {
    if (!view || !view->GetVisible())
      continue;

    auto width = view->GetPreferredSize().width();
    const gfx::Rect bounds(taken_left_width, 0, width, contents_height);
    view->SetBoundsRect(host_->GetMirroredRect(bounds));
    taken_left_width += width;
  }
  contents_width -= taken_left_width;

  int taken_right_width = 0;
  int right_side_x = contents_container->GetLocalBounds().right();
  for (auto* view : right_side_candidate_views) {
    if (!view || !view->GetVisible())
      continue;

    auto width = view->GetPreferredSize().width();
    right_side_x -= width;
    taken_right_width += width;
    const gfx::Rect bounds(right_side_x, 0, width, contents_height);
    view->SetBoundsRect(host_->GetMirroredRect(bounds));
  }
  contents_width -= taken_right_width;

  gfx::Size container_size(contents_width, contents_height);
  gfx::Rect new_devtools_bounds;
  gfx::Rect new_contents_bounds;

  ApplyDevToolsContentsResizingStrategy(
      strategy_, container_size, &new_devtools_bounds, &new_contents_bounds);

  new_devtools_bounds.Offset(taken_left_width, 0);
  new_contents_bounds.Offset(taken_left_width, 0);
  // DevTools cares about the specific position, so we have to compensate RTL
  // layout here.
  devtools_view_->SetBoundsRect(host_->GetMirroredRect(new_devtools_bounds));
  contents_view_->SetBoundsRect(host_->GetMirroredRect(new_contents_bounds));
}
