/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/brave_browser_frame_view_mac.h"

#include <memory>

#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "chrome/browser/ui/tabs/features.h"
#include "brave/browser/ui/views/frame/brave_non_client_hit_test_helper.h"
#include "brave/browser/ui/views/frame/brave_window_frame_graphic.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/view_ids.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "ui/base/hit_test.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/scoped_canvas.h"

BraveBrowserFrameViewMac::BraveBrowserFrameViewMac(
    BrowserWidget* browser_widget,
    BrowserView* browser_view)
    : BrowserFrameViewMac(browser_widget, browser_view) {
  auto* browser = browser_view->browser();
  frame_graphic_ =
      std::make_unique<BraveWindowFrameGraphic>(browser->profile());

  if (tabs::utils::SupportsVerticalTabs(browser)) {
    auto* prefs = browser->profile()->GetOriginalProfile()->GetPrefs();
    show_vertical_tabs_.Init(
        brave_tabs::kVerticalTabsEnabled, prefs,
        base::BindRepeating(
            &BraveBrowserFrameViewMac::UpdateWindowTitleAndControls,
            base::Unretained(this)));
    show_title_bar_on_vertical_tabs_.Init(
        brave_tabs::kVerticalTabsShowTitleOnWindow, prefs,
        base::BindRepeating(
            &BraveBrowserFrameViewMac::UpdateWindowTitleVisibility,
            base::Unretained(this)));
  }
}

BraveBrowserFrameViewMac::~BraveBrowserFrameViewMac() = default;

void BraveBrowserFrameViewMac::OnPaint(gfx::Canvas* canvas) {
  BrowserFrameViewMac::OnPaint(canvas);

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

int BraveBrowserFrameViewMac::GetTopInset(bool restored) const {
  if (tabs::utils::ShouldShowVerticalTabs(browser_view()->browser())) {
    if (ShouldShowWindowTitleForVerticalTabs()) {
      // Set minimum top inset to show caption buttons on frame.
      return 30;
    }

    // Bypassing BrowserFrameViewMac's implementation so that we don't have any
    // inset when hiding title bar.
    return 0;
  }

  if (!tabs::HorizontalTabsUpdateEnabled()) {
    return BrowserFrameViewMac::GetTopInset(restored);
  }

  return 0;
}

bool BraveBrowserFrameViewMac::ShouldShowWindowTitleForVerticalTabs() const {
  return tabs::utils::ShouldShowWindowTitleForVerticalTabs(
      browser_view()->browser());
}

void BraveBrowserFrameViewMac::UpdateWindowTitleVisibility() {
  if (!browser_view()->browser()->is_type_normal()) {
    return;
  }

  browser_widget()->SetWindowTitleVisibility(
      ShouldShowWindowTitleForVerticalTabs());
}

void BraveBrowserFrameViewMac::UpdateWindowTitleColor() {
  if (!browser_view()->browser()->is_type_normal()) {
    return;
  }

  browser_widget()->UpdateWindowTitleColor(
      GetCaptionColor(BrowserFrameActiveState::kUseCurrent));
}

int BraveBrowserFrameViewMac::NonClientHitTest(const gfx::Point& point) {
  if (auto res = brave::NonClientHitTest(browser_view(), point);
      res != HTNOWHERE) {
    return res;
  }

  return BrowserFrameViewMac::NonClientHitTest(point);
}

void BraveBrowserFrameViewMac::OnThemeChanged() {
  BrowserFrameViewMac::OnThemeChanged();
  UpdateWindowTitleColor();
}

void BraveBrowserFrameViewMac::UpdateWindowTitleAndControls() {
  UpdateWindowTitleVisibility();

  // In case title visibility wasn't changed and only vertical tab strip enabled
  // state changed, we should reset controls positions manually.
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&views::Widget::ResetWindowControlsPosition,
                                browser_widget()->GetWeakPtr()));
}

gfx::Size BraveBrowserFrameViewMac::GetMinimumSize() const {
  if (tabs::utils::ShouldShowVerticalTabs(browser_view()->browser())) {
    // In order to ignore tab strip height, skip BrowserFrameViewMac's
    // implementation.
    auto size = browser_widget()->client_view()->GetMinimumSize();
    size.SetToMax(gfx::Size(0, (size.width() * 3) / 4));
    // Note that we can't set empty bounds on Mac.
    size.SetToMax({1, 1});
    return size;
  }

  return BrowserFrameViewMac::GetMinimumSize();
}
