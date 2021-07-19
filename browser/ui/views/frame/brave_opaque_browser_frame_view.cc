/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/brave_opaque_browser_frame_view.h"

#include "brave/browser/ui/views/frame/brave_window_frame_graphic.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/opaque_browser_frame_view_layout.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/scoped_canvas.h"

BraveOpaqueBrowserFrameView::BraveOpaqueBrowserFrameView(
    BrowserFrame* frame,
    BrowserView* browser_view,
    OpaqueBrowserFrameViewLayout* layout)
    : OpaqueBrowserFrameView(frame, browser_view, layout) {
  frame_graphic_.reset(
      new BraveWindowFrameGraphic(browser_view->browser()->profile()));
}

BraveOpaqueBrowserFrameView::~BraveOpaqueBrowserFrameView() = default;

void BraveOpaqueBrowserFrameView::OnPaint(gfx::Canvas* canvas) {
  OpaqueBrowserFrameView::OnPaint(canvas);

  // Don't draw frame graphic over border.
  gfx::ScopedCanvas scoped_canvas(canvas);
  gfx::Rect bounds_to_frame_graphic(bounds());
  if (!IsFrameCondensed()) {
    bounds_to_frame_graphic.Inset(layout()->FrameBorderInsets(false).top(),
                                  layout()->FrameEdgeInsets(false).top());
    canvas->ClipRect(bounds_to_frame_graphic);
  }
  frame_graphic_->Paint(canvas, bounds_to_frame_graphic);
}
