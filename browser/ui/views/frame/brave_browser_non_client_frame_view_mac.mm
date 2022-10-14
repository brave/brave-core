/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "brave/browser/ui/views/frame/brave_browser_non_client_frame_view_mac.h"

#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/views/frame/brave_window_frame_graphic.h"
#include "brave/browser/ui/views/tabs/features.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/view_ids.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/scoped_canvas.h"

BraveBrowserNonClientFrameViewMac::BraveBrowserNonClientFrameViewMac(
    BrowserFrame* frame, BrowserView* browser_view)
    : BrowserNonClientFrameViewMac(frame, browser_view) {
  frame_graphic_ = std::make_unique<BraveWindowFrameGraphic>(
      browser_view->browser()->profile());

  if (tabs::features::ShouldShowVerticalTabs()) {
    show_title_bar_on_vertical_tabs_.Init(
        brave_tabs::kVerticalTabsShowTitleOnWindow,
        browser_view->browser()->profile()->GetOriginalProfile()->GetPrefs(),
        base::BindRepeating(
            &BraveBrowserNonClientFrameViewMac::UpdateWindowTitleVisibility,
            base::Unretained(this)));
  }
}

BraveBrowserNonClientFrameViewMac::~BraveBrowserNonClientFrameViewMac() = default;

void BraveBrowserNonClientFrameViewMac::OnPaint(gfx::Canvas* canvas) {
  BrowserNonClientFrameViewMac::OnPaint(canvas);

  // Don't draw frame graphic over border outline.
  gfx::ScopedCanvas scoped_canvas(canvas);
  gfx::Rect bounds_to_frame_graphic(bounds());
  if (!IsFrameCondensed()) {
    // Native frame has 1px border outline.
    constexpr int kFrameBorderOutlineThickness = 1;
    bounds_to_frame_graphic.Inset(gfx::Insets::VH(
        kFrameBorderOutlineThickness, kFrameBorderOutlineThickness));
    canvas->ClipRect(bounds_to_frame_graphic);
  }
  frame_graphic_->Paint(canvas, bounds_to_frame_graphic);
}

int BraveBrowserNonClientFrameViewMac::GetTopInset(bool restored) const {
  if (ShouldShowWindowTitleForVerticalTabs()) {
    // Set minimum top inset to show caption buttons on frame.
    return 30;
  }

  return BrowserNonClientFrameViewMac::GetTopInset(restored);
}

bool BraveBrowserNonClientFrameViewMac::ShouldShowWindowTitleForVerticalTabs()
    const {
  return tabs::features::ShouldShowWindowTitleForVerticalTabs(
      browser_view()->browser());
}

void BraveBrowserNonClientFrameViewMac::UpdateWindowTitleVisibility() {
  if (!browser_view()->browser()->is_type_normal())
    return;

  frame()->SetWindowTitleVisibility(ShouldShowWindowTitleForVerticalTabs());
}
