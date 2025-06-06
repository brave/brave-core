/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/split_view/brave_multi_contents_view.h"

#include "brave/browser/ui/brave_browser.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/browser/ui/views/frame/brave_contents_view_util.h"
#include "brave/browser/ui/views/split_view/split_view_location_bar.h"
#include "brave/browser/ui/views/split_view/split_view_separator.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/contents_web_view.h"
#include "chrome/browser/ui/views/frame/multi_contents_resize_area.h"
#include "chrome/browser/ui/views/frame/multi_contents_view_mini_toolbar.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/color/color_provider.h"
#include "ui/compositor/layer.h"
#include "ui/views/border.h"
#include "ui/views/widget/widget.h"

namespace {
constexpr auto kSpacingBetweenContentsWebViews = 4;
}  // namespace

BraveMultiContentsView::BraveMultiContentsView(
    BrowserView* browser_view,
    WebContentsFocusedCallback inactive_contents_focused_callback,
    WebContentsResizeCallback contents_resize_callback)
    : MultiContentsView(browser_view,
                        inactive_contents_focused_callback,
                        contents_resize_callback) {
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
  separator->SetPreferredSize(gfx::Size(kSpacingBetweenContentsWebViews, 0));
  resize_area_ = separator;
}

BraveMultiContentsView::~BraveMultiContentsView() = default;

void BraveMultiContentsView::UpdateContentsBorderAndOverlay() {
  if (!IsInSplitView()) {
    MultiContentsView::UpdateContentsBorderAndOverlay();
    return;
  }

  // Draw active/inactive outlines around the contents areas and updates mini
  // toolbar visibility.
  const auto set_contents_border_and_mini_toolbar =
      [this](ContentsContainerView* contents_container_view) {
        const bool is_active = contents_container_view->GetContentsView() ==
                               GetActiveContentsView();
        const float corner_radius = GetCornerRadius();
        if (is_active) {
          contents_container_view->SetBorder(views::CreateRoundedRectBorder(
              kBorderThickness, corner_radius,
              kColorBraveSplitViewActiveWebViewBorder));
        } else {
          contents_container_view->SetBorder(views::CreateBorderPainter(
              views::Painter::CreateRoundRectWith1PxBorderPainter(
                  GetColorProvider()->GetColor(
                      kColorBraveSplitViewInactiveWebViewBorder),
                  GetColorProvider()->GetColor(kColorToolbar), corner_radius,
                  SkBlendMode::kSrc,
                  /*anti_alias*/ true,
                  /*should_border_scale*/ true),
              gfx::Insets(kBorderThickness)));
        }
        // Chromium's Mini toolbar should be hidden always as we have our own
        // mini urlbar.
        contents_container_view->GetMiniToolbar()->SetVisible(false);
      };
  for (auto* contents_container_view : contents_container_views_) {
    set_contents_border_and_mini_toolbar(contents_container_view);
  }
}

void BraveMultiContentsView::Layout(PassKey) {
  // It's similar with upstream layout logic but replaced as we need to apply
  // different radius on each view.
  const gfx::Rect available_space(GetContentsBounds());
  ViewWidths widths = GetViewWidths(available_space);
  gfx::Rect start_rect(available_space.origin(),
                       gfx::Size(widths.start_width, available_space.height()));
  const gfx::Rect resize_rect(
      start_rect.top_right(),
      gfx::Size(widths.resize_width, available_space.height()));
  gfx::Rect end_rect(resize_rect.top_right(),
                     gfx::Size(widths.end_width, available_space.height()));
  gfx::RoundedCornersF corners(GetCornerRadius());
  for (auto* contents_container_view : contents_container_views_) {
    auto* contents_web_view = contents_container_view->GetContentsView();
    contents_web_view->layer()->SetRoundedCornerRadius(corners);
    contents_web_view->holder()->SetCornerRadii(corners);
    contents_web_view->holder()->SetCornerRadii(corners);
  }
  CHECK(contents_container_views_.size() == 2);
  contents_container_views_[0]->SetBoundsRect(start_rect);
  resize_area_->SetBoundsRect(resize_rect);
  contents_container_views_[1]->SetBoundsRect(end_rect);
}

void BraveMultiContentsView::SetActiveIndex(int index) {
  MultiContentsView::SetActiveIndex(index);

  UpdateSecondaryLocationBar();
}

float BraveMultiContentsView::GetCornerRadius() const {
  return BraveBrowser::ShouldUseBraveWebViewRoundedCorners(
             browser_view_->browser())
             ? BraveContentsViewUtil::kBorderRadius + kBorderThickness
             : 0;
}

void BraveMultiContentsView::OnDoubleClicked() {
  // Give same width on both contents view.
  contents_resize_callback_.Run(0.5);
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
      GetInactiveContentsView()->GetWebContents());
  secondary_location_bar_->SetParentWebView(
      contents_container_views_[inactive_index]);

  // Set separator's menu widget visibility after setting location bar's to make
  // separator's menu widget locate above the location bar.
  auto* separator = static_cast<SplitViewSeparator*>(resize_area_);
  CHECK(separator);
  separator->ShowMenuButtonWidget();
}

BEGIN_METADATA(BraveMultiContentsView)
END_METADATA
