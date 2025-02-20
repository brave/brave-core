/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/brave_contents_layout_manager.h"

#include "ui/views/view.h"

BraveContentsLayoutManager::BraveContentsLayoutManager(
    views::View* devtools_view,
    views::View* contents_view,
    views::View* watermark_view,
    views::View* reader_mode_toolbar)
    : ContentsLayoutManager(devtools_view, contents_view, watermark_view),
      contents_view_(contents_view),
      reader_mode_toolbar_(reader_mode_toolbar) {}

BraveContentsLayoutManager::~BraveContentsLayoutManager() = default;

views::ProposedLayout BraveContentsLayoutManager::CalculateProposedLayout(
    const views::SizeBounds& size_bounds) const {
  views::ProposedLayout layouts =
      ContentsLayoutManager::CalculateProposedLayout(size_bounds);

  if (!reader_mode_toolbar_) {
    return layouts;
  }

  auto* contents_layout = layouts.GetLayoutFor(contents_view_);
  if (!contents_layout) {
    return layouts;
  }

  if (reader_mode_toolbar_->GetVisible()) {
    gfx::Rect toolbar_bounds = contents_layout->bounds;
    toolbar_bounds.set_height(
        reader_mode_toolbar_->GetPreferredSize().height());
    contents_layout->bounds.Inset(
        gfx::Insets::TLBR(toolbar_bounds.height(), 0, 0, 0));

    layouts.child_layouts.emplace_back(
        reader_mode_toolbar_.get(), /*visible=*/true,
        host_view()->GetMirroredRect(toolbar_bounds),
        views::SizeBounds(layouts.host_size));
  } else {
    layouts.child_layouts.emplace_back(
        reader_mode_toolbar_.get(), /*visible=*/false,
        host_view()->GetMirroredRect(gfx::Rect()),
        views::SizeBounds(layouts.host_size));
  }

  return layouts;
}
