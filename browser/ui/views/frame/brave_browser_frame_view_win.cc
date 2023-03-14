/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/brave_browser_frame_view_win.h"

#include "brave/browser/ui/tabs/features.h"
#include "brave/browser/ui/views/frame/brave_non_client_hit_test_helper.h"
#include "brave/browser/ui/views/frame/brave_window_frame_graphic.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "ui/base/hit_test.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/scoped_canvas.h"

BraveBrowserFrameViewWin::BraveBrowserFrameViewWin(BrowserFrame* frame,
                                                   BrowserView* browser_view)
    : BrowserFrameViewWin(frame, browser_view) {
  frame_graphic_.reset(
      new BraveWindowFrameGraphic(browser_view->browser()->profile()));
}

BraveBrowserFrameViewWin::~BraveBrowserFrameViewWin() = default;

void BraveBrowserFrameViewWin::OnPaint(gfx::Canvas* canvas) {
  BrowserFrameViewWin::OnPaint(canvas);

  // Don't draw frame graphic over border outline.
  gfx::ScopedCanvas scoped_canvas(canvas);
  gfx::Rect bounds_to_frame_graphic(bounds());
  if (!IsFrameCondensed()) {
    // Native frame has 1px top border outline.
    constexpr int kFrameBorderOutlineThickness = 1;
    bounds_to_frame_graphic.Inset(
        gfx::Insets::VH(0, kFrameBorderOutlineThickness));
    canvas->ClipRect(bounds_to_frame_graphic);
  }
  frame_graphic_->Paint(canvas, bounds_to_frame_graphic);
}

int BraveBrowserFrameViewWin::GetTopInset(bool restored) const {
  if (!base::FeatureList::IsEnabled(tabs::features::kBraveVerticalTabs)) {
    return BrowserFrameViewWin::GetTopInset(restored);
  }

  if (auto* browser = browser_view()->browser();
      tabs::utils::ShouldShowVerticalTabs(browser) &&
      !tabs::utils::ShouldShowWindowTitleForVerticalTabs(browser)) {
    return 0;
  }

  return BrowserFrameViewWin::GetTopInset(restored);
}
int BraveBrowserFrameViewWin::NonClientHitTest(const gfx::Point& point) {
  if (auto res = brave::NonClientHitTest(browser_view(), point);
      res != HTNOWHERE) {
    return res;
  }

  return BrowserFrameViewWin::NonClientHitTest(point);
}
