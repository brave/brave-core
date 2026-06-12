/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/brave_browser_frame_view_mac.h"

#import <AppKit/AppKit.h>

#include <algorithm>
#include <memory>

#include "base/check_deref.h"
#include "base/functional/bind.h"
#include "base/memory/raw_ref.h"
#include "brave/browser/ui/focus_mode/focus_mode_controller.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/frame/brave_non_client_hit_test_helper.h"
#include "brave/browser/ui/views/frame/brave_window_frame_graphic.h"
#include "brave/browser/ui/views/frame/focus_mode_top_overlay.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
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
#include "ui/views/window/non_client_view.h"

namespace {

FocusModeTopOverlay* GetFocusModeTopOverlay(BrowserView* browser_view) {
  if (!browser_view) {
    return nullptr;
  }
  return BraveBrowserView::From(browser_view)->focus_mode_top_overlay();
}

}  // namespace

class BraveBrowserFrameViewMac::ScopedFocusModeDisable {
 public:
  explicit ScopedFocusModeDisable(FocusModeController* controller)
      : controller_(CHECK_DEREF(controller)),
        was_enabled_(controller->IsEnabled()) {
    controller_->SetEnabled(false);
  }

  ~ScopedFocusModeDisable() {
    if (was_enabled_) {
      controller_->SetEnabled(true);
    }
  }

 private:
  const raw_ref<FocusModeController> controller_;
  bool was_enabled_;
};

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

  if (auto* controller = browser->GetFeatures().focus_mode_controller()) {
    focus_mode_observation_.Observe(controller);
    if (auto* overlay = GetFocusModeTopOverlay(browser_view)) {
      overlay_reveal_subscription_ =
          overlay->RegisterRevealFractionChanged(base::BindRepeating(
              &BraveBrowserFrameViewMac::OnTopOverlayRevealFractionChanged,
              base::Unretained(this)));
    }
  }

  compact_horizontal_tabs_.Init(
      brave_tabs::kCompactHorizontalTabs, g_browser_process->local_state(),
      base::BindRepeating(
          &BraveBrowserFrameViewMac::UpdateWindowTitleAndControls,
          base::Unretained(this)));
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
                                  tabs::GetHorizontalTabButtonYOffset());
  // Compact: zero the caption button vertical margins so the leading
  // exclusion collapses around the buttons themselves, letting the shorter
  // tab strip sit centred against the traffic lights.
  if (tabs::UseCompactHorizontalTabs() &&
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

void BraveBrowserFrameViewMac::UpdateWindowControlsOpacity() {
  auto* widget = GetWidget();
  if (!widget) {
    return;
  }

  auto* window = widget->GetNativeWindow().GetNativeNSWindow();
  if (!window) {
    return;
  }

  double reveal_fraction = 1.0;
  if (auto* overlay = GetFocusModeTopOverlay(GetBrowserView())) {
    reveal_fraction = overlay->GetRevealFraction();
  }

  // Hold the buttons hidden while the overlay is mostly closed, then fade
  // them in linearly across the last (1 - kFadeStart) of the reveal so the
  // lights "pop in" near the end of the slide rather than tracking it
  // continuously.
  constexpr double kFadeStart = 0.7;
  const double alpha =
      std::clamp((reveal_fraction - kFadeStart) / (1.0 - kFadeStart), 0.0, 1.0);

  for (NSWindowButton kind :
       {NSWindowCloseButton, NSWindowMiniaturizeButton, NSWindowZoomButton}) {
    [[window standardWindowButton:kind] setAlphaValue:alpha];
  }
}

void BraveBrowserFrameViewMac::OnTopOverlayRevealFractionChanged(
    double reveal_fraction) {
  UpdateWindowControlsOpacity();
}

void BraveBrowserFrameViewMac::ResetWindowControlsPosition() {
  browser_widget()->ResetWindowControlsPosition();
  UpdateWindowControlsOpacity();
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
  auto* browser_view = GetBrowserView();
  auto* browser = browser_view->browser();
  if (!ImmersiveModeController::From(browser)->IsEnabled()) {
    auto* non_client_hit_test_helper =
        browser->browser_window_features()->brave_non_client_hit_test_helper();
    if (auto res =
            non_client_hit_test_helper->NonClientHitTest(browser_view, point);
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
      FROM_HERE,
      base::BindOnce(&BraveBrowserFrameViewMac::ResetWindowControlsPosition,
                     weak_factory_.GetWeakPtr()));
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

void BraveBrowserFrameViewMac::OnFullscreenStateChanged() {
  // When entering fullscreen ensure that focus mode is disabled. Focus mode and
  // immersive fullscreen on MacOS have conflicting presentation requirements,
  // and both modes want to move the tabstrip into different parent views.
  // Disabling focus mode before immersive mode is enabled ensures that the
  // tabstrip view is returned to the expected placement before the immersive
  // controller attempts to reparent it.
  if (GetBrowserView()->IsFullscreen()) {
    if (!scoped_focus_mode_disable_) {
      auto* browser = GetBrowserView()->browser();
      if (auto* controller = browser->GetFeatures().focus_mode_controller()) {
        scoped_focus_mode_disable_ =
            std::make_unique<ScopedFocusModeDisable>(controller);
      }
    }
  } else {
    scoped_focus_mode_disable_.reset();
  }
  BrowserFrameViewMac::OnFullscreenStateChanged();
}

void BraveBrowserFrameViewMac::OnFocusModeToggled(bool enabled) {
  UpdateWindowTitleAndControls();
  UpdateWindowControlsOpacity();
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
