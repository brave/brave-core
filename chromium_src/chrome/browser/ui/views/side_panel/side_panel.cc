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

// Rename the upstream RemoveHeaderView implementation so we can provide
// our own method. Upstream RemoveHeaderView() resets Border always but we
// want to preserve the no-border state. As we don't set any header to all
// panels, making it empty would not add any side-effect.
#define RemoveHeaderView RemoveHeaderView_UnUsed

#include <chrome/browser/ui/views/side_panel/side_panel.cc>

#undef RemoveHeaderView

#endif  // BUILDFLAG(ENABLE_SIDEBAR_V2)

#if BUILDFLAG(ENABLE_SIDEBAR_V2)

void SidePanel::VisibilityChanged(View* starting_from, bool is_visible) {
  AccessiblePaneView::VisibilityChanged(starting_from, is_visible);

  // Whenever open/closes, upstream resets border style.
  // Set our border style again.
  if (GetVisible()) {
    UpdateBorder();
  }
}

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
  if (!border_view_) {
    return;
  }

  if (rounded_border_enabled_) {
    // Upstream GetBorderInsets() has a negative top to overlap the toolbar;
    // Brave doesn't need that overlap.
    SetBorder(views::CreateEmptyBorder(GetBorderInsets().set_top(0)));
  } else {
    SetBorder(nullptr);
  }

  border_view_->SetVisible(rounded_border_enabled_);
}

void SidePanel::RemoveHeaderView() {
  // See above method overriding's comment why it's empty.
}

#endif  // BUILDFLAG(ENABLE_SIDEBAR_V2)
