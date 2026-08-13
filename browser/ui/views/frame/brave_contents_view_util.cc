/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/brave_contents_view_util.h"

#include "base/check.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/layout_constants.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/rounded_corners_f.h"
#include "ui/views/border.h"

// static
std::unique_ptr<views::Border>
BraveContentsViewUtil::CreateContentsOutlineBorder(
    const ui::ColorProvider* color_provider,
    const gfx::RoundedCornersF& corner_radii) {
  CHECK(color_provider);
  return views::CreateBorderPainter(
      views::Painter::CreateRoundRectWith1PxBorderPainter(
          color_provider->GetColor(kColorToolbar),
          color_provider->GetColor(kColorBraveContentsOutline), corner_radii,
          SkBlendMode::kSrc,
          /*antialias=*/true,
          /*should_border_scale=*/true),
      gfx::Insets(kRoundedCornersContentsOutlineThickness));
}

// static
int BraveContentsViewUtil::GetRoundedCornersWebViewMargin(Browser* browser) {
  return BraveBrowserView::ShouldUseBraveWebViewRoundedCornersForContents(
             browser)
             ? kRoundedCornersContentsViewMargin
             : 0;
}

// static
int BraveContentsViewUtil::GetRoundedCornersWebViewMargin(
    const Browser* browser) {
  return BraveBrowserView::ShouldUseBraveWebViewRoundedCornersForContents(
             browser)
             ? kRoundedCornersContentsViewMargin
             : 0;
}
