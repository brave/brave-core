/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/brave_browser_frame_view_mac.h"

#include <memory>

#include "brave/browser/ui/tabs/brave_compact_horizontal_tabs_layout.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/views/frame/brave_non_client_hit_test_helper.h"
#include "brave/browser/ui/views/frame/brave_window_frame_graphic.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/fullscreen_util_mac.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/tabs/features.h"
#include "chrome/browser/ui/view_ids.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/immersive_mode_controller.h"
#include "ui/base/hit_test.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/outsets_f.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/scoped_canvas.h"

BraveBrowserFrameViewMac::BraveBrowserFrameViewMac(
    BrowserWidget* browser_widget,
    BrowserView* browser_view)
    : BrowserFrameViewMac(browser_widget, browser_view) {
  auto* browser = browser_view->browser();
  frame_graphic_ =
      std::make_unique<BraveWindowFrameGraphic>(browser->profile());

  if (tabs::utils::SupportsBraveVerticalTabs(browser)) {
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

gfx::Rect BraveBrowserFrameViewMac::GetBoundsForClientView() const {
  if (ShouldShowWindowTitleForVerticalTabs()) {
    // Upstream implementation doesn't properly use
    // GetBrowserLayoutParams().visual_client_area. They just use bounds() for
    // client view bounds which results in make client area fills the entire
    // frame bounds. So it doesn't help even if we override
    // GetBrowserLayoutParams(). to inset the visual_client_area. So we need to
    // manually inset the client view bounds here.
    gfx::Rect client_bounds = bounds();
    client_bounds.Inset(gfx::Insets::TLBR(GetTopInset(false), 0, 0, 0));
    return client_bounds;
  }
  return BrowserFrameViewMac::GetBoundsForClientView();
}

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
  if (tabs::utils::ShouldShowBraveVerticalTabs(GetBrowserView()->browser())) {
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

BrowserFrameViewMac::BoundsAndMargins
BraveBrowserFrameViewMac::GetCaptionButtonBounds() const {
  auto bounds_and_margins = BrowserFrameViewMac::GetCaptionButtonBounds();
  bounds_and_margins.bounds.set_y(bounds_and_margins.bounds.y() +
                                  tabs::GetHorizontalTabControlsDelta());
  // In compact horizontal tabs mode the tab pill is ~26 DIP tall, but the
  // upstream caption button rect carries vertical margins of 11 px (macOS 15-)
  // / 10 px (macOS 26+) above and below. Those margins flow into
  // `BrowserFrameView::GetBrowserLayoutParams()` as `leading_exclusion`'s
  // vertical extent, reserving more vertical band than the tab strip occupies
  // and biasing where the tab strip lays out next to the traffic lights. Zero
  // them so the exclusion rect collapses around the buttons themselves and
  // the tab strip can sit centred against them. Mirrors Helium's
  // `fix-caption-button-bounds.patch` (imputnet/helium @ b6e5b77e).
  if (tabs::ShouldUseCompactHorizontalTabsForNonTouchUI() &&
      !bounds_and_margins.bounds.IsEmpty()) {
    bounds_and_margins.margins.set_top(0).set_bottom(0);
  }
  return bounds_and_margins;
}

bool BraveBrowserFrameViewMac::ShouldShowWindowTitleForVerticalTabs() const {
  return tabs::utils::ShouldShowWindowTitleForVerticalTabs(
             GetBrowserView()->browser()) &&
         !GetBrowserView()->IsFullscreen();
}

void BraveBrowserFrameViewMac::UpdateWindowTitleVisibility() {
  if (!GetBrowserView()->browser()->is_type_normal()) {
    return;
  }

  browser_widget()->SetWindowTitleVisibility(
      ShouldShowWindowTitleForVerticalTabs());
}

void BraveBrowserFrameViewMac::UpdateWindowTitleColor() {
  if (!GetBrowserView()->browser()->is_type_normal()) {
    return;
  }

  browser_widget()->UpdateWindowTitleColor(
      GetCaptionColor(BrowserFrameActiveState::kUseCurrent));
}

int BraveBrowserFrameViewMac::NonClientHitTest(const gfx::Point& point) {
  // During window teardown or fullscreen transitions on Mac, AppKit can trigger
  // hit tests via windowDidBecomeKey:/makeKeyAndOrderFront: while the widget is
  // in a partially destroyed state. Guard against this to prevent crashes.
  if (!GetWidget() || GetWidget()->IsClosed()) {
    return HTNOWHERE;
  }

  // In immersive fullscreen the toolbar is reparented to a separate overlay
  // widget, violating brave::NonClientHitTest's precondition that the toolbar
  // and browser view share the same widget. Skip it while immersive mode is
  // active.
  if (!ImmersiveModeController::From(GetBrowserView()->browser())
           ->IsEnabled()) {
    if (auto res = brave::NonClientHitTest(GetBrowserView(), point);
        res != HTNOWHERE) {
      return res;
    }
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
  if (tabs::utils::ShouldShowBraveVerticalTabs(GetBrowserView()->browser())) {
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

bool BraveBrowserFrameViewMac::ShouldHideTopUIInFullscreen() const {
  // When a browser window starts with horizontal tabs and the user switches to
  // vertical tabs at runtime, fullscreen_toolbar_controller_ in the base class
  // is nil — it was skipped at construction because
  // UsesImmersiveFullscreenMode() returned true at that point. Messaging nil in
  // ObjC returns 0, which equals TOOLBAR_PRESENT, so the base implementation
  // incorrectly reports "don't hide" during tab (content) fullscreen. Intercept
  // that case explicitly.
  if (tabs::utils::ShouldShowBraveVerticalTabs(GetBrowserView()->browser()) &&
      fullscreen_utils::IsInContentFullscreen(GetBrowserView()->browser())) {
    return true;
  }
  return BrowserFrameViewMac::ShouldHideTopUIInFullscreen();
}
