/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/split_view/brave_multi_contents_view.h"

#include "brave/browser/ui/brave_browser.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/browser/ui/views/frame/brave_contents_view_util.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/contents_web_view.h"
#include "chrome/browser/ui/views/frame/multi_contents_resize_area.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/color/color_provider.h"
#include "ui/compositor/layer.h"
#include "ui/views/border.h"

BraveMultiContentsView::~BraveMultiContentsView() = default;

void BraveMultiContentsView::UpdateContentsBorder() {
  if (!IsInSplitView()) {
    MultiContentsView::UpdateContentsBorder();
    return;
  }

  auto* cp = GetColorProvider();
  if (!cp) {
    return;
  }

  // Draw active/inactive outlines around the contents areas.
  const auto set_contents_border =
      [this, cp](ContentsContainerView* contents_container_view) {
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
                  cp->GetColor(kColorBraveSplitViewInactiveWebViewBorder),
                  cp->GetColor(kColorToolbar), corner_radius, SkBlendMode::kSrc,
                  /*anti_alias*/ true,
                  /*should_border_scale*/ true),
              gfx::Insets(kBorderThickness)));
        }
      };
  for (auto* contents_container_view : contents_container_views_) {
    set_contents_border(contents_container_view);
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

float BraveMultiContentsView::GetCornerRadius() const {
  return BraveBrowser::ShouldUseBraveWebViewRoundedCorners(
             browser_view_->browser())
             ? BraveContentsViewUtil::kBorderRadius + kBorderThickness
             : 0;
}

BEGIN_METADATA(BraveMultiContentsView)
END_METADATA
