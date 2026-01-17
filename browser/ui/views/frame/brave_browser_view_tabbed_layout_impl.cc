/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/brave_browser_view_tabbed_layout_impl.h"

#include "chrome/browser/ui/views/frame/custom_corners_background.h"
#include "chrome/browser/ui/views/frame/layout/browser_view_layout_delegate.h"

BraveBrowserViewTabbedLayoutImpl::BraveBrowserViewTabbedLayoutImpl(
    std::unique_ptr<BrowserViewLayoutDelegate> delegate,
    Browser* browser,
    BrowserViewLayoutViews views)
    : BrowserViewTabbedLayoutImpl(std::move(delegate),
                                  browser,
                                  std::move(views)) {}

BraveBrowserViewTabbedLayoutImpl::~BraveBrowserViewTabbedLayoutImpl() = default;

void BraveBrowserViewTabbedLayoutImpl::DoPostLayoutVisualAdjustments(
    const BrowserLayoutParams& params) {
  BrowserViewTabbedLayoutImpl::DoPostLayoutVisualAdjustments(params);
  if (delegate().ShouldDrawVerticalTabStrip()) {
    return;
  }

  auto* const toolbar_background =
      static_cast<CustomCornersBackground*>(views().toolbar->background());
  CustomCornersBackground::Corners toolbar_corners;
  toolbar_corners.upper_trailing.type =
      CustomCornersBackground::CornerType::kRoundedWithBackground;
  toolbar_corners.upper_leading.type =
      CustomCornersBackground::CornerType::kRoundedWithBackground;
  toolbar_background->SetCorners(toolbar_corners);
}
