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
#include "brave/browser/ui/views/split_view/split_view_separator.h"
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
  // Replace upstream's resize area with ours.
  // To prevent making |resize_area_| dangling pointer,
  // reset it after setting null to |resize_area_|.
  {
    std::unique_ptr<views::View> resize_area = RemoveChildViewT(resize_area_);
    resize_area_ = nullptr;
  }
  auto* separator = AddChildView(
      std::make_unique<SplitViewSeparator>(browser_view_->browser()));
  separator->set_resize_delegate(this);
  separator->set_separator_delegate(this);
  separator->SetPreferredSize(
      gfx::Size(kSpacingBetweenContentsContainerViews, 0));
  resize_area_ = separator;
  start_contents_view_inset_ = gfx::Insets();
  end_contents_view_inset_ = gfx::Insets();
}

BraveMultiContentsView::~BraveMultiContentsView() = default;

void BraveMultiContentsView::Layout(PassKey) {
  LayoutSuperclass<MultiContentsView>(this);

  BraveBrowserView::From(browser_view_)->NotifyDialogPositionRequiresUpdate();
}

void BraveMultiContentsView::OnDoubleClicked() {
  // Give same width on both contents view.
  delegate_->ResizeWebContents(0.5);
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

  // Set separator's menu widget visibility after setting location bar's to make
  // separator's menu widget locate above the location bar.
  auto* separator = static_cast<SplitViewSeparator*>(resize_area_);
  CHECK(separator);
  separator->ShowMenuButtonWidget();
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
