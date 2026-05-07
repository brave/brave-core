/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/brave_opaque_browser_frame_view.h"

#include <memory>

#include "base/check.h"
#include "base/functional/bind.h"
#include "brave/browser/ui/focus_mode/focus_mode_utils.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/frame/brave_non_client_hit_test_helper.h"
#include "brave/browser/ui/views/frame/brave_window_frame_graphic.h"
#include "brave/browser/ui/views/frame/focus_mode_top_overlay.h"
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
#include "ui/gfx/geometry/transform.h"
#include "ui/gfx/scoped_canvas.h"
#include "ui/views/controls/button/button.h"

namespace {

FocusModeTopOverlay* GetFocusModeTopOverlay(BrowserView* browser_view) {
  if (!browser_view) {
    return nullptr;
  }
  return BraveBrowserView::From(browser_view)->focus_mode_top_overlay();
}

}  // namespace

BraveOpaqueBrowserFrameView::BraveOpaqueBrowserFrameView(
    BrowserWidget* frame,
    BrowserView* browser_view,
    OpaqueBrowserFrameViewLayout* layout)
    : OpaqueBrowserFrameView(frame, browser_view, layout) {
  auto* browser = browser_view->browser();
  DCHECK(browser);
  frame_graphic_ =
      std::make_unique<BraveWindowFrameGraphic>(browser->profile());

  if (auto* controller = browser->GetFeatures().focus_mode_controller()) {
    focus_mode_observation_.Observe(controller);
    if (auto* overlay = GetFocusModeTopOverlay(browser_view)) {
      overlay_reveal_subscription_ =
          overlay->RegisterRevealFractionChanged(base::BindRepeating(
              &BraveOpaqueBrowserFrameView::OnTopOverlayRevealFractionChanged,
              base::Unretained(this)));
    }
  }
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
  bool captions_fully_revealed = true;
  if (auto* overlay = GetFocusModeTopOverlay(GetBrowserView())) {
    captions_fully_revealed = overlay->GetRevealFraction() == 1.0;
  }

  if (tabs::utils::ShouldShowBraveVerticalTabs(GetBrowserView()->browser()) &&
      captions_fully_revealed) {
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

  if (auto res = GetBrowserView()
                     ->browser()
                     ->browser_window_features()
                     ->brave_non_client_hit_test_helper()
                     ->NonClientHitTest(GetBrowserView(), point);
      res != HTNOWHERE) {
    return res;
  }

  int result = OpaqueBrowserFrameView::NonClientHitTest(point);

  if (!captions_fully_revealed) {
    if (result == HTMINBUTTON || result == HTMAXBUTTON || result == HTCLOSE) {
      result = HTCAPTION;
    }
  }

  return result;
}

bool BraveOpaqueBrowserFrameView::ShouldCaptionButtonsPaintToLayer() const {
  auto* browser = GetBrowserView()->browser();
  if (IsFocusModeEnabled(browser)) {
    return true;
  }
  return tabs::utils::ShouldShowBraveVerticalTabs(browser) &&
         !tabs::utils::ShouldShowWindowTitleForVerticalTabs(browser);
}

void BraveOpaqueBrowserFrameView::RefreshCaptionButtonLayers() {
  const bool paint_to_layer = ShouldCaptionButtonsPaintToLayer();

  for (views::Button* button :
       {close_button_.get(), restore_button_.get(), maximize_button_.get(),
        minimize_button_.get()}) {
    if (!button) {
      continue;
    }

    if (paint_to_layer) {
      if (!button->layer()) {
        button->SetPaintToLayer();
        button->layer()->SetFillsBoundsOpaquely(false);
      }
    } else if (button->layer()) {
      button->DestroyLayer();
    }
  }
}

void BraveOpaqueBrowserFrameView::OnFocusModeToggled(bool enabled) {
  RefreshCaptionButtonLayers();
}

void BraveOpaqueBrowserFrameView::OnTopOverlayRevealFractionChanged(
    double reveal_fraction) {
  for (views::Button* button :
       {close_button_.get(), restore_button_.get(), maximize_button_.get(),
        minimize_button_.get()}) {
    if (!button || !button->layer()) {
      continue;
    }

    const int height = button->height();
    button->layer()->SetTransform(
        gfx::Transform::MakeTranslation(0, -height * (1.0 - reveal_fraction)));
  }
}

void BraveOpaqueBrowserFrameView::
    UpdateCaptionButtonPlaceholderContainerBackground() {
  OpaqueBrowserFrameView::UpdateCaptionButtonPlaceholderContainerBackground();

  RefreshCaptionButtonLayers();

  // Notify toolbar view that caption button's width changed so that it can
  // make space for caption buttons.
  static_cast<BraveToolbarView*>(GetBrowserView()->toolbar())
      ->UpdateHorizontalPadding();
}

void BraveOpaqueBrowserFrameView::Layout(PassKey key) {
  LayoutSuperclass<OpaqueBrowserFrameView>(this);

  // Why frame view should ask toolbar's padding update?
  // This kind of exclusion should be handled by BrowserViewLayout.
  // BraveBrowserFrameViewLinuxNative::Layout() does same thing.
  // TODO(https://github.com/brave/brave-browser/issues/55209):
  // Investigate what's happening.
  static_cast<BraveToolbarView*>(GetBrowserView()->toolbar())
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
  DCHECK(GetBrowserView());
  auto* browser = GetBrowserView()->browser();
  DCHECK(browser);
  return tabs::utils::ShouldShowBraveVerticalTabs(browser);
}

BEGIN_METADATA(BraveOpaqueBrowserFrameView)
END_METADATA
