/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_OPAQUE_BROWSER_FRAME_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_OPAQUE_BROWSER_FRAME_VIEW_H_

#include <memory>

#include "chrome/browser/ui/views/frame/opaque_browser_frame_view.h"
#include "ui/base/metadata/metadata_header_macros.h"

class BraveWindowFrameGraphic;
class FocusModeRevealObserver;

class BraveOpaqueBrowserFrameView : public OpaqueBrowserFrameView {
  METADATA_HEADER(BraveOpaqueBrowserFrameView, OpaqueBrowserFrameView)
 public:
  BraveOpaqueBrowserFrameView(BrowserWidget* frame,
                              BrowserView* browser_view,
                              OpaqueBrowserFrameViewLayout* layout);
  ~BraveOpaqueBrowserFrameView() override;

  BraveOpaqueBrowserFrameView(const BraveOpaqueBrowserFrameView&) = delete;
  BraveOpaqueBrowserFrameView& operator=(
      const BraveOpaqueBrowserFrameView&) = delete;

  // OpaqueBrowserFrameView overrides:
  void OnPaint(gfx::Canvas* canvas) override;
  int NonClientHitTest(const gfx::Point& point) override;
  void UpdateCaptionButtonPlaceholderContainerBackground() override;
  void PaintClientEdge(gfx::Canvas* canvas) const override;
  int GetTopInset(bool restored) const override;
  int GetTopAreaHeight() const override;
  void Layout(PassKey) override;

 private:
  bool ShouldShowVerticalTabs() const;

  // Returns true when each caption button should be promoted to its own layer
  // (paint-to-layer + non-opaque). True in either of two overlapping cases:
  // (a) the vertical-tabs / no-title "buttons over toolbar" mode, where the
  // buttons need to composite over a non-frame background, and (b) focus
  // mode, where the buttons need a layer transform for the slide animation.
  // Used to keep `UpdateCaptionButtonPlaceholderContainerBackground` and the
  // focus-mode reveal handler from clobbering each other's layer state.
  bool ShouldCaptionButtonsPaintToLayer() const;
  void RefreshCaptionButtonLayers();

  // Called by `focus_mode_reveal_observer_` when focus mode is toggled.
  void OnFocusModeToggled(bool enabled);

  // Drives a per-button slide animation in lockstep with `FocusModeTopOverlay`.
  // When `fraction` is 1.0 the buttons sit at their laid-out positions; as
  // `fraction` approaches 0.0 each button is translated upward by its own
  // height so it slides off the top of the frame.
  void ApplyFocusModeRevealFraction(double fraction);

  std::unique_ptr<BraveWindowFrameGraphic> frame_graphic_;

  // True while focus mode is enabled on this window. Latched by
  // `OnFocusModeToggled`.
  bool focus_mode_active_ = false;

  // True unless focus mode is mid-animation or hiding the caption buttons.
  // Gates the caption-button hit-test paths in `NonClientHitTest`: while
  // false, button hits are routed to `HTCAPTION` so the visually-absent
  // buttons aren't clickable. View hit testing is bounds-based and ignores
  // layer transforms, so this gate is required during the slide.
  bool focus_mode_caption_revealed_ = true;

  std::unique_ptr<FocusModeRevealObserver> focus_mode_reveal_observer_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_OPAQUE_BROWSER_FRAME_VIEW_H_
