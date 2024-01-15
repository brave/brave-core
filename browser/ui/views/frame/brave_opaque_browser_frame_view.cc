/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/brave_opaque_browser_frame_view.h"

#include <memory>

#include "brave/browser/ui/views/frame/brave_non_client_hit_test_helper.h"
#include "brave/browser/ui/views/frame/brave_window_frame_graphic.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "brave/browser/ui/views/toolbar/brave_toolbar_view.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/opaque_browser_frame_view_layout.h"
#include "ui/base/hit_test.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/compositor/layer.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/scoped_canvas.h"

BraveOpaqueBrowserFrameView::BraveOpaqueBrowserFrameView(
    BrowserFrame* frame,
    BrowserView* browser_view,
    OpaqueBrowserFrameViewLayout* layout)
    : OpaqueBrowserFrameView(frame, browser_view, layout) {
  frame_graphic_ = std::make_unique<BraveWindowFrameGraphic>(
      browser_view->browser()->profile());
}

BraveOpaqueBrowserFrameView::~BraveOpaqueBrowserFrameView() = default;

void BraveOpaqueBrowserFrameView::OnPaint(gfx::Canvas* canvas) {
  OpaqueBrowserFrameView::OnPaint(canvas);

  // Don't draw frame graphic over border.
  gfx::ScopedCanvas scoped_canvas(canvas);
  gfx::Rect bounds_to_frame_graphic(bounds());
  if (!IsFrameCondensed()) {
    bounds_to_frame_graphic.Inset(
        gfx::Insets::VH(layout()->FrameBorderInsets(false).top(),
                        layout()->FrameEdgeInsets(false).top()));
    canvas->ClipRect(bounds_to_frame_graphic);
  }
  frame_graphic_->Paint(canvas, bounds_to_frame_graphic);
}

int BraveOpaqueBrowserFrameView::NonClientHitTest(const gfx::Point& point) {
  if (tabs::utils::ShouldShowVerticalTabs(browser_view()->browser())) {
    auto hit_test_caption_button = [](views::Button* button,
                                      const gfx::Point& point) {
      return button && button->GetVisible() &&
             button->GetMirroredBounds().Contains(point);
    };

    if (hit_test_caption_button(close_button_, point)) {
      return HTCLOSE;
    }
    if (hit_test_caption_button(restore_button_, point)) {
      return HTMAXBUTTON;
    }
    if (hit_test_caption_button(maximize_button_, point)) {
      return HTMAXBUTTON;
    }
    if (hit_test_caption_button(minimize_button_, point)) {
      return HTMINBUTTON;
    }
  }

  if (auto res = brave::NonClientHitTest(browser_view(), point);
      res != HTNOWHERE) {
    return res;
  }

  return OpaqueBrowserFrameView::NonClientHitTest(point);
}

void BraveOpaqueBrowserFrameView::
    UpdateCaptionButtonPlaceholderContainerBackground() {
  OpaqueBrowserFrameView::UpdateCaptionButtonPlaceholderContainerBackground();

  DCHECK(browser_view());
  auto* browser = browser_view()->browser();
  DCHECK(browser);
  const bool should_window_caption_buttons_overlap_toolbar =
      tabs::utils::ShouldShowVerticalTabs(browser) &&
      !tabs::utils::ShouldShowWindowTitleForVerticalTabs(browser);

  for (auto& button :
       {close_button_, restore_button_, maximize_button_, minimize_button_}) {
    if (!button) {
      continue;
    }

    if (should_window_caption_buttons_overlap_toolbar) {
      auto* cp = GetColorProvider();
      DCHECK(cp);

      button->SetPaintToLayer();
      button->layer()->SetFillsBoundsOpaquely(false);
    } else {
      if (button->layer()) {
        button->DestroyLayer();
      }
    }
  }

  // Notify toolbar view that caption button's width changed so that it can
  // make space for caption buttons.
  static_cast<BraveToolbarView*>(browser_view()->toolbar())
      ->UpdateHorizontalPadding();
}

void BraveOpaqueBrowserFrameView::PaintClientEdge(gfx::Canvas* canvas) const {
  // Don't draw client edge next to toolbar when it's in vertical tab strip mode
  if (ShouldShowVerticalTabs()) {
    return;
  }

  OpaqueBrowserFrameView::PaintClientEdge(canvas);
}

int BraveOpaqueBrowserFrameView::GetTopInset(bool restored) const {
  if (ShouldShowVerticalTabs()) {
    // In order to ignore horizontal tab strip's height, we by pass the base
    // class's implementation.
    return layout_->NonClientTopHeight(restored);
  }

  return OpaqueBrowserFrameView::GetTopInset(restored);
}

int BraveOpaqueBrowserFrameView::GetTopAreaHeight() const {
  if (ShouldShowVerticalTabs()) {
    // In order to ignore horizontal tab strip's height, we by pass the base
    // class's implementation.
    return layout_->NonClientTopHeight(false);
  }

  return OpaqueBrowserFrameView::GetTopAreaHeight();
}

bool BraveOpaqueBrowserFrameView::ShouldShowVerticalTabs() const {
  DCHECK(browser_view());
  auto* browser = browser_view()->browser();
  DCHECK(browser);
  return tabs::utils::ShouldShowVerticalTabs(browser);
}

BEGIN_METADATA(BraveOpaqueBrowserFrameView)
END_METADATA
