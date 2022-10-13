/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/brave_contents_layout_manager.h"

#include "ui/views/view.h"

BraveContentsLayoutManager::BraveContentsLayoutManager(
    views::View* devtools_view,
    views::View* contents_view,
    views::View* sidebar_container_view,
    views::View* vertical_tabs_container)
    : ContentsLayoutManager(devtools_view, contents_view),
      sidebar_container_view_(sidebar_container_view),
      vertical_tabs_container_(vertical_tabs_container) {
  DCHECK(sidebar_container_view_);
}

BraveContentsLayoutManager::~BraveContentsLayoutManager() = default;

void BraveContentsLayoutManager::Layout(views::View* contents_container) {
  DCHECK(host_ == contents_container);

  int height = contents_container->height();
  int width = contents_container->width();

  int taken_width = 0;
  for (const auto view : {sidebar_container_view_, vertical_tabs_container_}) {
    if (!view || !view->GetVisible())
      continue;

    auto preferred_width = view->GetPreferredSize().width();
    const gfx::Rect bounds(taken_width, 0, preferred_width, height);
    view->SetBoundsRect(host_->GetMirroredRect(bounds));
    taken_width += preferred_width;
  }

  gfx::Size container_size(width - taken_width, height);
  gfx::Rect new_devtools_bounds;
  gfx::Rect new_contents_bounds;

  ApplyDevToolsContentsResizingStrategy(
      strategy_, container_size, &new_devtools_bounds, &new_contents_bounds);

  new_devtools_bounds.Offset(taken_width, 0);
  new_contents_bounds.Offset(taken_width, 0);
  // DevTools cares about the specific position, so we have to compensate RTL
  // layout here.
  devtools_view_->SetBoundsRect(host_->GetMirroredRect(new_devtools_bounds));
  contents_view_->SetBoundsRect(host_->GetMirroredRect(new_contents_bounds));
}
