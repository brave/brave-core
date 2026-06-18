// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "base/i18n/rtl.h"
#include "brave/browser/ui/views/side_panel/side_panel_utils.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/views/side_panel/side_panel.h"
#include "ui/compositor/layer.h"

// Rename upstream methods so we can provide thin wrappers that reapply
// rounded-corner and border state.
#define AddHeaderView AddHeaderView_ChromiumImpl
#define RemoveHeaderView RemoveHeaderView_ChromiumImpl

#include <chrome/browser/ui/views/side_panel/side_panel.cc>

#undef RemoveHeaderView
#undef AddHeaderView

namespace {

// Applies the current rounded-corner values to every direct child of the
// content parent view. Used when the pref changes or the panel opens with
// existing content (so OnChildViewAdded never fired with the new values).
void UpdateContentWrapperChildCorners(views::View* content_parent_view,
                                      BrowserView* browser_view) {
  // ContentParentView hosts multiple content view and shows at once.
  CHECK(content_parent_view->GetUseDefaultFillLayout());

  auto corners = brave::GetPanelContentsRoundedCorners(browser_view);
  for (views::View* child : content_parent_view->children()) {
    // If the child is a WebView or paints to a layer, round its corners.
    if (views::IsViewClass<views::WebView>(child)) {
      views::AsViewClass<views::WebView>(child)->holder()->SetCornerRadii(
          corners);
    }
    // Try to detect if the child is a views::View wrapper of a WebView. If so,
    // round its corners.
    if (child->children().size() == 1 &&
        views::IsViewClass<views::WebView>(child->children()[0])) {
      views::AsViewClass<views::WebView>(child->children()[0])
          ->holder()
          ->SetCornerRadii(corners);
    }
    if (child->layer()) {
      child->layer()->SetIsFastRoundedCorner(true);
      child->layer()->SetRoundedCornerRadius(corners);
    }
  }
}

}  // namespace

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

  // Re-apply corners to existing content: the pref or header state changed.
  UpdateContentWrapperChildCorners(GetContentParentView(), browser_view_);

  const int header_top_inset =
      header_view_ ? header_view_->GetPreferredSize().height() : 0;
  gfx::Insets insets;
  insets.set_top(header_top_inset);

  // With rounded corners the content view owns its own margins, so only add an
  // outer-side gap for visual separation from the window chrome.
  const bool is_sidebar_leading = (IsRightAligned() == base::i18n::IsRTL());
  if (rounded_border_enabled_) {
    insets.set_bottom(kRoundedCornersContentsViewMargin);
    if (is_sidebar_leading) {
      insets.set_left(kRoundedCornersContentsViewMargin);
    } else {
      insets.set_right(kRoundedCornersContentsViewMargin);
    }
    SetBorder(views::CreateEmptyBorder(insets));
    return;
  }

  // Without rounded corners, draw a 1px vertical separator between content and
  // panel.
  constexpr int kBorderThickness = 1;
  if (is_sidebar_leading) {
    insets.set_right(kBorderThickness);
  } else {
    insets.set_left(kBorderThickness);
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
