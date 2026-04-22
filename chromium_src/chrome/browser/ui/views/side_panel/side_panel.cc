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

// Rename the upstream Open/Close implementation so we can provide a thin
// wrapper that reapplies border state after UpdateVisibility() runs.
// UpdateVisibility() unconditionally re-shows border_view_ whenever the panel
// opens, which would undo SetBorderEnabled(false).
#define Open Open_ChromiumImpl
#define Close Close_ChromiumImpl

// Rename the upstream RemoveHeaderView implementation so we can provide
// a thin wrapper that reapplies the border state after it runs.
// Upstream RemoveHeaderView() sets Border always but we want to preserve
// the no-border state if it is set.
#define RemoveHeaderView RemoveHeaderView_ChromiumImpl

#include <chrome/browser/ui/views/side_panel/side_panel.cc>

#undef RemoveHeaderView
#undef Close
#undef Open

#endif  // BUILDFLAG(ENABLE_SIDEBAR_V2)

#if BUILDFLAG(ENABLE_SIDEBAR_V2)

void SidePanel::Open(bool animated) {
  Open_ChromiumImpl(animated);

  // UpdateVisibility() unconditionally sets border_view_ visible when opening;
  // restore desired state.
  UpdateBorder();
}

void SidePanel::Close(bool animated) {
  Close_ChromiumImpl(animated);

  // UpdateVisibility() unconditionally sets border_view_ visible when closing;
  // restore desired state.
  UpdateBorder();
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

void SidePanel::SetBorderEnabled(bool enabled) {
  if (border_enabled_ == enabled) {
    return;
  }

  border_enabled_ = enabled;

  if (GetVisible()) {
    UpdateBorder();
  }
}

void SidePanel::UpdateBorder() {
  // No border or not initialized yet.
  if (!border_view_ || !border_view_->layer()) {
    return;
  }

  if (border_enabled_) {
    // Upstream GetBorderInsets() has a negative top to overlap the toolbar;
    // Brave doesn't need that overlap.
    SetBorder(views::CreateEmptyBorder(GetBorderInsets().set_top(0)));
  } else {
    SetBorder(nullptr);
  }

  border_view_->SetVisible(border_enabled_);
}

void SidePanel::RemoveHeaderView() {
  // Do nothing here as we don't set header view.
}

#endif  // BUILDFLAG(ENABLE_SIDEBAR_V2)
