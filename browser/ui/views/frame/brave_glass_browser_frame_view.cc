/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/brave_glass_browser_frame_view.h"

#include "brave/browser/ui/views/frame/brave_window_frame_graphic.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/scoped_canvas.h"

BraveGlassBrowserFrameView::BraveGlassBrowserFrameView(
    BrowserFrame* frame, BrowserView* browser_view)
    : GlassBrowserFrameView(frame, browser_view) {
  frame_graphic_.reset(
      new BraveWindowFrameGraphic(browser_view->browser()->profile()));
}

BraveGlassBrowserFrameView::~BraveGlassBrowserFrameView() = default;

void BraveGlassBrowserFrameView::OnPaint(gfx::Canvas* canvas) {
  GlassBrowserFrameView::OnPaint(canvas);

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
