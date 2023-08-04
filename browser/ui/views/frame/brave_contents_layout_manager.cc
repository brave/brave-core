/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/brave_contents_layout_manager.h"

#include <algorithm>
#include <limits>

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

  const bool reader_mode_toolbar_visible =
      reader_mode_toolbar_view_ && reader_mode_toolbar_view_->GetVisible();

  // Use upstream layout logic when sidebar and reader mode toolbar aren't
  // shown.
  if (!sidebar_container_view_->GetVisible() && !reader_mode_toolbar_visible) {
    return ContentsLayoutManager::Layout(contents_container);
  }

  int proposed_sidebar_width =
      sidebar_container_view_->GetVisible() ? CalculateTargetSideBarWidth() : 0;
  const int contents_width =
      contents_container->width() - proposed_sidebar_width;

  int sidebar_x = 0;
  if (!sidebar_on_left_) {
    sidebar_x =
        contents_container->GetLocalBounds().right() - proposed_sidebar_width;
  }

  const int contents_height = contents_container->height();
  const gfx::Rect bounds(sidebar_x, 0, proposed_sidebar_width, contents_height);
  sidebar_container_view_->SetBoundsRect(host_->GetMirroredRect(bounds));

#if BUILDFLAG(IS_MAC)
  // On Mac, we shouldn't set empty rect for web view. That could cause crash
  // from StatusBubbleViews. As StatusBubbleViews width is one third of the
  // base view, sets 3 here so that StatusBubbleViews can have at least 1 width.
  gfx::Size container_size(contents_width > 0 ? contents_width : 3,
                           contents_height);
#else
  gfx::Size container_size(contents_width, contents_height);
#endif
  gfx::Rect new_devtools_bounds;
  gfx::Rect new_contents_bounds;

  ApplyDevToolsContentsResizingStrategy(
      strategy_, container_size, &new_devtools_bounds, &new_contents_bounds);

  new_devtools_bounds.Offset(sidebar_on_left_ ? proposed_sidebar_width : 0, 0);
  new_contents_bounds.Offset(sidebar_on_left_ ? proposed_sidebar_width : 0, 0);

  gfx::Rect reader_mode_toolbar_bounds;
  if (reader_mode_toolbar_visible) {
    reader_mode_toolbar_bounds.SetRect(
        new_contents_bounds.x(), 0, new_contents_bounds.width(),
        reader_mode_toolbar_view_->GetPreferredSize().height());
    reader_mode_toolbar_view_->SetBoundsRect(
        host_->GetMirroredRect(reader_mode_toolbar_bounds));
    new_contents_bounds.set_y(reader_mode_toolbar_bounds.height());
    new_contents_bounds.set_height(contents_height -
                                   reader_mode_toolbar_bounds.height());
  }

  // DevTools cares about the specific position, so we have to compensate RTL
  // layout here.
  devtools_view_->SetBoundsRect(host_->GetMirroredRect(new_devtools_bounds));
  contents_view_->SetBoundsRect(host_->GetMirroredRect(new_contents_bounds));
}

int BraveContentsLayoutManager::CalculateTargetSideBarWidth() const {
  int proposed_sidebar_width =
      sidebar_container_view_->GetPreferredSize().width();
  const int contents_width = host_->width();

  if (proposed_sidebar_width == std::numeric_limits<int>::max()) {
    // Takes up the entire space for fullscreen.
    return contents_width;
  }

  // Guarantee 20% width for contents at least.
  if ((contents_width - proposed_sidebar_width) <= contents_width * 0.2) {
    proposed_sidebar_width = std::min(static_cast<int>(contents_width * 0.8),
                                      proposed_sidebar_width);
  }

  return proposed_sidebar_width;
}
