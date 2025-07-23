/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/split_view/brave_contents_container_view.h"

#include "brave/browser/ui/brave_browser.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/browser/ui/views/frame/brave_contents_view_util.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/exclusive_access/exclusive_access_manager.h"
#include "chrome/browser/ui/exclusive_access/fullscreen_controller.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/multi_contents_view_mini_toolbar.h"
#include "chrome/browser/ui/views/frame/scrim_view.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/color/color_provider.h"
#include "ui/compositor/layer.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/views/border.h"

BraveContentsContainerView::BraveContentsContainerView(
    BrowserView* browser_view)
    : ContentsContainerView(browser_view), browser_view_(*browser_view) {}

BraveContentsContainerView::~BraveContentsContainerView() = default;

void BraveContentsContainerView::UpdateBorderAndOverlay(bool is_in_split,
                                                        bool is_active,
                                                        bool show_scrim) {
  // We don't show scrim view always.
  GetScrimView()->SetVisible(false);

  // We have our own secondary toolbar.
  GetMiniToolbar()->SetVisible(false);

  gfx::RoundedCornersF contents_corner_radius(GetCornerRadius(false));
  auto* contents_web_view = GetContentsView();
  contents_web_view->layer()->SetRoundedCornerRadius(contents_corner_radius);
  if (contents_web_view->holder()->native_view()) {
    contents_web_view->holder()->SetCornerRadii(contents_corner_radius);
  }

  if (!is_in_split) {
    SetBorder(nullptr);
    return;
  }

  // Draw active/inactive outlines around the contents areas and updates mini
  // toolbar visibility.
  const float border_corner_radius(GetCornerRadius(true));
  if (is_active) {
    SetBorder(views::CreateRoundedRectBorder(
        kBorderThickness, border_corner_radius,
        kColorBraveSplitViewActiveWebViewBorder));
  } else {
    SetBorder(views::CreateBorderPainter(
        views::Painter::CreateRoundRectWith1PxBorderPainter(
            GetColorProvider()->GetColor(
                kColorBraveSplitViewInactiveWebViewBorder),
            GetColorProvider()->GetColor(kColorToolbar), border_corner_radius,
            SkBlendMode::kSrc,
            /*anti_alias*/ true,
            /*should_border_scale*/ true),
        gfx::Insets(kBorderThickness)));
  }
}

float BraveContentsContainerView::GetCornerRadius(bool for_border) const {
  auto* exclusive_access_manager =
      browser_view_->browser()->GetFeatures().exclusive_access_manager();
  if (exclusive_access_manager &&
      exclusive_access_manager->fullscreen_controller()->IsTabFullscreen()) {
    return 0;
  }

  return BraveBrowser::ShouldUseBraveWebViewRoundedCorners(
             browser_view_->browser())
             ? BraveContentsViewUtil::kBorderRadius +
                   (for_border ? kBorderThickness : 0)
             : 0;
}

BEGIN_METADATA(BraveContentsContainerView)
END_METADATA
