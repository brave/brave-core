// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/sidebar/buildflags/buildflags.h"
#include "build/buildflag.h"

#if BUILDFLAG(ENABLE_SIDEBAR_V2)
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/layout_constants.h"
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
  // Add at the end so the resize area is above content_parent_view_ in
  // z-order. The strip always overlaps the content edge and must win the
  // hit-test to receive drag events.
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
  // To make sure |horizontal_alignemnt_| refreshed before using
  // IsRightAligned().
  UpdateHorizontalAlignment();

  const int header_top_inset =
      header_view_ ? header_view_->GetPreferredSize().height() : 0;
  gfx::Insets insets;
  insets.set_top(header_top_inset);

  // With rounded corners the content view owns its own margins, so only add an
  // outer-side gap for visual separation from the window chrome.
  if (rounded_border_enabled_) {
    insets.set_bottom(kRoundedCornersContentsViewMargin);
    if (IsRightAligned()) {
      insets.set_right(kRoundedCornersContentsViewMargin);
    } else {
      insets.set_left(kRoundedCornersContentsViewMargin);
    }
    SetBorder(views::CreateEmptyBorder(insets));
    return;
  }

  // Without rounded corners, draw a 1px vertical separator between content and
  // panel.
  constexpr int kBorderThickness = 1;
  if (IsRightAligned()) {
    insets.set_left(kBorderThickness);
  } else {
    insets.set_right(kBorderThickness);
  }
  SetBorder(
      views::CreateSolidSidedBorder(insets, kColorToolbarContentAreaSeparator));
}

void SidePanel::AddHeaderView(std::unique_ptr<views::View> view) {
  AddHeaderView_ChromiumImpl(std::move(view));
  UpdateBorder();

  // The resize area overlaps other views (contents, header), so it must be
  // top-most to receive drag events.
  ReorderChildView(resize_area_, children().size());
}

void SidePanel::RemoveHeaderView() {
  RemoveHeaderView_ChromiumImpl();
  UpdateBorder();
}

#endif  // BUILDFLAG(ENABLE_SIDEBAR_V2)
