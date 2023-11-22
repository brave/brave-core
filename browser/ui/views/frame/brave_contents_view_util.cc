/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/brave_contents_view_util.h"

#include "ui/compositor/layer.h"
#include "ui/views/view.h"

namespace {

constexpr ViewShadow::ShadowParameters kShadow{
    .offset_x = 0,
    .offset_y = 1,
    .blur_radius = 4,
    .shadow_color = SkColorSetA(SK_ColorBLACK, 0.07 * 255)};

}  // namespace

std::unique_ptr<ViewShadow> BraveContentsViewUtil::CreateShadow(
    views::View* view) {
  DCHECK(view);
  auto shadow = std::make_unique<ViewShadow>(view, kBorderRadius, kShadow);
  view->layer()->SetRoundedCornerRadius(gfx::RoundedCornersF(kBorderRadius));
  view->layer()->SetIsFastRoundedCorner(true);
  return shadow;
}
