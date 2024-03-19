/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/brave_browser_frame_view_win.h"

#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/views/frame/brave_non_client_hit_test_helper.h"
#include "brave/browser/ui/views/frame/brave_window_frame_graphic.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/views/frame/browser_caption_button_container_win.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/win/titlebar_config.h"
#include "ui/base/hit_test.h"
#include "ui/compositor/layer.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/scoped_canvas.h"

BraveBrowserFrameViewWin::BraveBrowserFrameViewWin(BrowserFrame* frame,
                                                   BrowserView* browser_view)
    : BrowserFrameViewWin(frame, browser_view) {
  frame_graphic_.reset(
      new BraveWindowFrameGraphic(browser_view->browser()->profile()));

  DCHECK(browser_view->browser());
  auto* prefs = browser_view->browser()->profile()->GetPrefs();
  using_vertical_tabs_.Init(
      brave_tabs::kVerticalTabsEnabled, prefs,
      base::BindRepeating(&BraveBrowserFrameViewWin::OnVerticalTabsPrefsChanged,
                          base::Unretained(this)));
  showing_window_title_for_vertical_tabs_.Init(
      brave_tabs::kVerticalTabsShowTitleOnWindow, prefs,
      base::BindRepeating(&BraveBrowserFrameViewWin::OnVerticalTabsPrefsChanged,
                          base::Unretained(this)));
}

BraveBrowserFrameViewWin::~BraveBrowserFrameViewWin() = default;

bool BraveBrowserFrameViewWin::ShouldCaptionButtonsBeDrawnOverToolbar() const {
  auto* browser = browser_view()->browser();
  return tabs::utils::ShouldShowVerticalTabs(browser) &&
         !tabs::utils::ShouldShowWindowTitleForVerticalTabs(browser);
}

void BraveBrowserFrameViewWin::OnVerticalTabsPrefsChanged() {
  caption_button_container_->UpdateButtons();
  caption_button_container_->InvalidateLayout();
  LayoutCaptionButtons();
}

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
  if (auto* browser = browser_view()->browser();
      tabs::utils::ShouldShowVerticalTabs(browser)) {
    if (!tabs::utils::ShouldShowWindowTitleForVerticalTabs(browser)) {
      if (auto* widget = GetWidget(); !widget || !widget->IsMaximized()) {
        return 0;
      }

      // In case maximized with Mica enabled, we should return system borders
      // thickness.
      return ShouldBrowserCustomDrawTitlebar(browser_view())
                 ? 0
                 : FrameTopBorderThickness(/*restored*/ false);
    }

    if (!ShouldBrowserCustomDrawTitlebar(browser_view())) {
      // In case Mica enabled, we should extend top insets so that title bar can
      // be visible.
      return TopAreaHeight(restored) +
             caption_button_container_->GetPreferredSize().height();
    }
  }

  return BrowserFrameViewWin::GetTopInset(restored);
}

int BraveBrowserFrameViewWin::NonClientHitTest(const gfx::Point& point) {
  auto result = BrowserFrameViewWin::NonClientHitTest(point);
  if (result != HTCLIENT) {
    return result;
  }

  if (caption_button_container_) {
    // When we use custom caption button container, it could return HTCLIENT.
    // We shouldn't override it.
    gfx::Point local_point = point;
    ConvertPointToTarget(parent(), caption_button_container_, &local_point);
    if (caption_button_container_->HitTestPoint(local_point)) {
      const int hit_test_result =
          caption_button_container_->NonClientHitTest(local_point);
      if (hit_test_result != HTNOWHERE) {
        return hit_test_result;
      }
    }
  }

  if (auto overridden_result = brave::NonClientHitTest(browser_view(), point);
      overridden_result != HTNOWHERE) {
    return overridden_result;
  }

  return result;
}

bool BraveBrowserFrameViewWin::ShouldShowWindowTitle(TitlebarType type) const {
  if (auto* browser = browser_view()->browser();
      tabs::utils::ShouldShowVerticalTabs(browser) &&
      tabs::utils::ShouldShowWindowTitleForVerticalTabs(browser) &&
      type == TitlebarType::kCustom &&
      !ShouldBrowserCustomDrawTitlebar(browser_view())) {
    // When using Mica, title won't be drawn by the OS. In this case, we
    // should use our custom title
    // TODO(sko) Possibly, there's code that setting HWND wndclass that
    // prevents the OS from drawing the title
    return true;
  }

  return BrowserFrameViewWin::ShouldShowWindowTitle(type);
}

void BraveBrowserFrameViewWin::LayoutCaptionButtons() {
  BrowserFrameViewWin::LayoutCaptionButtons();

  // This may look pretty weird because we're laying out
  // |caption_button_container_| while ShouldBrowserCustomDrawTitlebar()
  // is false. This is because when Win11's Mica titlebar is enabled, we need to
  // show custom caption buttons over toolbar. We're forcing them visible in
  // chromium_src/.../browser_caption_button_container_win.cc
  if (ShouldCaptionButtonsBeDrawnOverToolbar() &&
      !ShouldBrowserCustomDrawTitlebar(browser_view())) {
    caption_button_container_->SetX(
        CaptionButtonsOnLeadingEdge()
            ? 0
            : width() - caption_button_container_->width());
  }
}
