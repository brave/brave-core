/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/brave_contents_view_util.h"

#include "base/check.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "ui/compositor/layer.h"
#include "ui/views/view.h"

std::unique_ptr<ViewShadow> BraveContentsViewUtil::CreateShadow(
    views::View* view) {
  static const ViewShadow::ShadowParameters kShadow{
      .offset_x = 0,
      .offset_y = 0,
      .blur_radius = BraveContentsViewUtil::GetMargin(),
      .shadow_color = SkColorSetA(SK_ColorBLACK, 0.1 * 255)};

  DCHECK(view);
  auto shadow = std::make_unique<ViewShadow>(view, GetBorderRadius(), kShadow);
  view->layer()->SetRoundedCornerRadius(
      gfx::RoundedCornersF(GetBorderRadius()));
  view->layer()->SetIsFastRoundedCorner(true);
  return shadow;
}

int BraveContentsViewUtil::GetRoundedCornersWebViewMargin(Browser* browser) {
  return BraveBrowserView::ShouldUseBraveWebViewRoundedCornersForContents(
             browser)
             ? BraveContentsViewUtil::GetMargin()
             : 0;
}

#if !BUILDFLAG(IS_MAC)

// static
int BraveContentsViewUtil::GetMargin() {
  return 4;
}

// static
int BraveContentsViewUtil::GetBorderRadius() {
  return 2;
}

#endif
