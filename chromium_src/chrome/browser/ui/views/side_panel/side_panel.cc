// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/sidebar/buildflags/buildflags.h"
#include "build/buildflag.h"

#if BUILDFLAG(ENABLE_SIDEBAR_V2)
// Include the Brave-overridden header first, WITHOUT the rename macros below
// active, so that the class declaration keeps the original Open name and the
// include guard prevents the header from being re-processed when the upstream
// .cc pulls it in.
#include "chrome/browser/ui/views/side_panel/side_panel.h"

// Rename the upstream Add/RemoveHeaderView implementation so we can provide
// a thin wrapper that reapplies border state.
#define AddHeaderView AddHeaderView_ChromiumImpl
#define RemoveHeaderView RemoveHeaderView_ChromiumImpl

#include <chrome/browser/ui/views/side_panel/side_panel.cc>

#undef RemoveHeaderView
#undef AddHeaderView

#endif  // BUILDFLAG(ENABLE_SIDEBAR_V2)

#if BUILDFLAG(ENABLE_SIDEBAR_V2)

void SidePanel::SetResizeArea(std::unique_ptr<views::View> resize_area) {
  CHECK(resize_area);
  auto old_resize_area = RemoveChildViewT(resize_area_);
  // Add at the end so the resize area is above content_parent_view_ in z-order.
  // This is required for the no-border case where the resize strip overlaps
  // the content edge and must win the hit-test.
  resize_area_ = AddChildView(std::move(resize_area));
  resize_area_->InsertBeforeInFocusList(content_parent_view_);
}

void SidePanel::SetRoundedBorderEnabled(bool enabled) {
  if (rounded_border_enabled_ == enabled) {
    return;
  }

  rounded_border_enabled_ = enabled;

  if (GetVisible()) {
    UpdateBorder();
  }
}

void SidePanel::UpdateBorder() {
  // When a Brave header is attached, reserve top inset for it so the header
  // paints over the border strip without overlapping content.
  const int header_top_inset =
      header_view_ ? header_view_->GetPreferredSize().height() : 0;

  if (rounded_border_enabled_) {
    // Upstream GetBorderInsets() has a negative top to overlap the toolbar;
    // Brave doesn't need that overlap.
    SetBorder(
        views::CreateEmptyBorder(GetBorderInsets().set_top(header_top_inset)));
  } else {
    SetBorder(
        views::CreateEmptyBorder(gfx::Insets::TLBR(header_top_inset, 0, 0, 0)));
  }
}

void SidePanel::AddHeaderView(std::unique_ptr<views::View> view) {
  AddHeaderView_ChromiumImpl(std::move(view));
  UpdateBorder();

  // Resize area is overlapped with other views such as contents and
  // header view. Make it top-most view to get event for dragging.
  if (!rounded_border_enabled_) {
    ReorderChildView(resize_area_, children().size());
  }
}

void SidePanel::RemoveHeaderView() {
  RemoveHeaderView_ChromiumImpl();
  UpdateBorder();
}

#endif  // BUILDFLAG(ENABLE_SIDEBAR_V2)
