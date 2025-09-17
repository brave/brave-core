/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/split_view/brave_multi_contents_view.h"

#include <utility>

#include "base/check.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/frame/split_view/brave_contents_container_view.h"
#include "brave/browser/ui/views/split_view/split_view_location_bar.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/multi_contents_resize_area.h"
#include "chrome/browser/ui/views/frame/multi_contents_view_delegate.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/views/widget/widget.h"

namespace {
constexpr auto kSpacingBetweenContentsContainerViews = 4;
}  // namespace

// static
BraveMultiContentsView* BraveMultiContentsView::From(MultiContentsView* view) {
  CHECK(view);
  return static_cast<BraveMultiContentsView*>(view);
}

BraveMultiContentsView::BraveMultiContentsView(
    BrowserView* browser_view,
    std::unique_ptr<MultiContentsViewDelegate> delegate)
    : MultiContentsView(browser_view, std::move(delegate)) {
  resize_area_->SetPreferredSize(
      gfx::Size(kSpacingBetweenContentsContainerViews, 0));
  start_contents_view_inset_ = gfx::Insets();
  end_contents_view_inset_ = gfx::Insets();
}

BraveMultiContentsView::~BraveMultiContentsView() = default;

void BraveMultiContentsView::Layout(PassKey) {
  LayoutSuperclass<MultiContentsView>(this);

  BraveBrowserView::From(browser_view_)->NotifyDialogPositionRequiresUpdate();
}

void BraveMultiContentsView::UseContentsContainerViewForWebPanel() {
  if (!contents_container_view_for_web_panel_) {
    contents_container_view_for_web_panel_ = AddChildView(
        std::make_unique<BraveContentsContainerView>(browser_view_));
    contents_container_view_for_web_panel_->SetVisible(false);
  }
}

void BraveMultiContentsView::SetWebPanelVisible(bool visible) {
  CHECK(contents_container_view_for_web_panel_);
  contents_container_view_for_web_panel_->SetVisible(visible);
  contents_container_view_for_web_panel_->UpdateBorderAndOverlay(true, true,
                                                                 false);
}

bool BraveMultiContentsView::IsWebPanelVisible() const {
  CHECK(contents_container_view_for_web_panel_);
  return contents_container_view_for_web_panel_->GetVisible();
}

void BraveMultiContentsView::SetWebPanelWidth(int width) {
  web_panel_width_ = width;
  InvalidateLayout();
}

void BraveMultiContentsView::SetWebPanelOnLeft(bool left) {
  web_panel_on_left_ = left;
  InvalidateLayout();
}

views::ProposedLayout BraveMultiContentsView::CalculateProposedLayout(
    const views::SizeBounds& size_bounds) const {
  if (!size_bounds.is_fully_bounded() ||
      !contents_container_view_for_web_panel_) {
    return MultiContentsView::CalculateProposedLayout(size_bounds);
  }

  views::SizeBounds new_size_bounds = size_bounds;
  const int web_panel_width = GetWebPanelWidth();

  // Negative to shrink to make room for web panel.
  new_size_bounds.Enlarge(-web_panel_width, 0);
  views::ProposedLayout layouts =
      MultiContentsView::CalculateProposedLayout(new_size_bounds);

  if (web_panel_on_left_) {
    for (auto& layout : layouts.child_layouts) {
      // Move all other views to right to put web panel on left side.
      layout.bounds.Offset(web_panel_width, 0);
    }
  }

  int host_width = size_bounds.width().value();
  int host_height = size_bounds.height().value();
  const int web_panel_x = web_panel_on_left_ ? 0 : host_width - web_panel_width;
  gfx::Rect web_panel_rect(web_panel_x, 0, web_panel_width, host_height);
  layouts.child_layouts.emplace_back(
      contents_container_view_for_web_panel_.get(),
      contents_container_view_for_web_panel_->GetVisible(), web_panel_rect);

  layouts.host_size = gfx::Size(host_width, host_height);
  return layouts;
}

void BraveMultiContentsView::ResetResizeArea() {
  // Give same width on both contents view.
  // Pass true to make delegate save ratio in session service like resizing
  // complete.
  delegate_->ResizeWebContents(0.5, /*done_resizing=*/true);
}

int BraveMultiContentsView::GetWebPanelWidth() const {
  CHECK(contents_container_view_for_web_panel_);

  if (!contents_container_view_for_web_panel_->GetVisible()) {
    return 0;
  }

  return web_panel_width_;
}

void BraveMultiContentsView::UpdateSecondaryLocationBar() {
  if (!secondary_location_bar_) {
    secondary_location_bar_ = std::make_unique<SplitViewLocationBar>(
        browser_view_->browser()->profile()->GetPrefs());
    secondary_location_bar_widget_ = std::make_unique<views::Widget>();

    secondary_location_bar_widget_->Init(
        SplitViewLocationBar::GetWidgetInitParams(
            GetWidget()->GetNativeView(), secondary_location_bar_.get()));
  }

  // Inactive web contents/view should be set to secondary location bar
  // as it's attached to inactive contents view.
  int inactive_index = active_index_ == 0 ? 1 : 0;
  secondary_location_bar_->SetWebContents(
      GetInactiveContentsView()->web_contents());
  secondary_location_bar_->SetParentWebView(
      contents_container_views_[inactive_index]);
}

void BraveMultiContentsView::UpdateCornerRadius() {
  UpdateContentsBorderAndOverlay();
}

BraveContentsContainerView*
BraveMultiContentsView::GetActiveContentsContainerView() {
  return BraveContentsContainerView::From(
      contents_container_views_[active_index_]);
}

BraveContentsContainerView*
BraveMultiContentsView::GetInactiveContentsContainerView() {
  return BraveContentsContainerView::From(
      contents_container_views_[GetInactiveIndex()]);
}

BEGIN_METADATA(BraveMultiContentsView)
END_METADATA
