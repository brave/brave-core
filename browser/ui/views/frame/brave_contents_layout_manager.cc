/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/brave_contents_layout_manager.h"

#include "ui/views/view.h"

BraveContentsLayoutManager::BraveContentsLayoutManager(
    views::View* sidebar_container_view,
    views::View* vertical_tabs_container,
    views::View* contents_container_view)
    : sidebar_container_view_(sidebar_container_view),
      vertical_tabs_container_(vertical_tabs_container),
      contents_container_view_(contents_container_view) {
  DCHECK(sidebar_container_view_ || vertical_tabs_container_)
      << "At least one of these should be valid";
  DCHECK(contents_container_view_);
}

BraveContentsLayoutManager::~BraveContentsLayoutManager() = default;

void BraveContentsLayoutManager::Layout(views::View* host) {
  DCHECK(host_ == host);

  int height = host->height();
  int width = host->width();

  int x = 0;
  for (const auto view : {sidebar_container_view_, vertical_tabs_container_}) {
    if (!view)
      continue;

    gfx::Rect bounds(x, 0, view->GetPreferredSize().width(), height);
    view->SetBoundsRect(bounds);
    x = bounds.right();
    width -= bounds.width();
  }

  gfx::Rect contents_bounds(x, 0, width, height);
  contents_container_view_->SetBoundsRect(
      host_->GetMirroredRect(contents_bounds));
}

gfx::Size BraveContentsLayoutManager::GetPreferredSize(
    const views::View* host) const {
  return gfx::Size();
}

void BraveContentsLayoutManager::Installed(views::View* host) {
  DCHECK(!host_);
  host_ = host;
}
