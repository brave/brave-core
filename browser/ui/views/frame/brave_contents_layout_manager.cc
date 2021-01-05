/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/brave_contents_layout_manager.h"

#include "ui/views/view.h"

BraveContentsLayoutManager::BraveContentsLayoutManager(
    views::View* sidebar_container_view,
    views::View* contents_container_view)
    : sidebar_container_view_(sidebar_container_view),
      contents_container_view_(contents_container_view) {
  DCHECK(sidebar_container_view_);
  DCHECK(contents_container_view_);
}

BraveContentsLayoutManager::~BraveContentsLayoutManager() = default;

void BraveContentsLayoutManager::Layout(views::View* host) {
  DCHECK(host_ == host);

  int height = host->height();
  int width = host->width();

  const int sidebar_width = sidebar_container_view_->GetPreferredSize().width();
  const gfx::Rect sidebar_bounds(0, 0, sidebar_width, height);
  gfx::Rect contents_bounds(sidebar_width, 0, width - sidebar_width, height);

  sidebar_container_view_->SetBoundsRect(
      host_->GetMirroredRect(sidebar_bounds));
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
