/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/brave_contents_web_view.h"

#include <memory>
#include <utility>

#include "brave/browser/ui/views/view_shadow.h"
#include "ui/views/border.h"

namespace {

constexpr int kContentsWebViewBorderRadius = 4;

constexpr gfx::Insets kContentsWebViewPadding = gfx::Insets::TLBR(0, 4, 4, 4);

constexpr ViewShadow::ShadowParameters kContentsShadow{
    .offset_x = 0,
    .offset_y = 1,
    .blur_radius = 4,
    .shadow_color = SkColorSetA(SK_ColorGREEN, 0.7 * 255)};

}  // namespace

BraveContentsWebView::~BraveContentsWebView() = default;

BraveContentsWebView::BraveContentsWebView(
    content::BrowserContext* browser_context)
    : ContentsWebView(browser_context) {
  SetBorder(views::CreateEmptyBorder(kContentsWebViewPadding));
}

void BraveContentsWebView::RenderViewReady() {
  holder()->SetCornerRadii(gfx::RoundedCornersF(kContentsWebViewBorderRadius));
  shadow_ = std::make_unique<ViewShadow>(
      holder(), kContentsWebViewBorderRadius, kContentsShadow);
  ContentsWebView::RenderViewReady();
}
