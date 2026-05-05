/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_BROWSER_FRAME_VIEW_WIN_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_BROWSER_FRAME_VIEW_WIN_H_

#include <memory>

#include "chrome/browser/ui/views/frame/browser_frame_view_win.h"
#include "components/prefs/pref_member.h"

class BraveWindowFrameGraphic;
class FocusModeRevealObserver;

class BraveBrowserFrameViewWin : public BrowserFrameViewWin {
 public:
  BraveBrowserFrameViewWin(BrowserWidget* browser_widget,
                           BrowserView* browser_view);
  ~BraveBrowserFrameViewWin() override;

  BraveBrowserFrameViewWin(const BraveBrowserFrameViewWin&) = delete;
  BraveBrowserFrameViewWin& operator=(const BraveBrowserFrameViewWin&) = delete;

  bool ShouldCaptionButtonsBeDrawnOverToolbar() const;

 private:
  void OnVerticalTabsPrefsChanged();

  // Drives the caption button container's slide animation in lockstep with
  // `FocusModeTopOverlay`'s reveal fraction. When `fraction` is 1.0 the
  // container sits at its laid-out position; as `fraction` approaches 0.0 the
  // container is translated upward by its own height so it slides off the top
  // of the frame.
  void ApplyFocusModeRevealFraction(double fraction);

  // BraveBrowserFrameViewWin overrides:
  void OnPaint(gfx::Canvas* canvas) override;
  int GetTopInset(bool restored) const override;
  int NonClientHitTest(const gfx::Point& point) override;
  bool ShouldShowWindowTitle(TitlebarType type) const override;
  void LayoutCaptionButtons() override;

  std::unique_ptr<BraveWindowFrameGraphic> frame_graphic_;

  BooleanPrefMember using_vertical_tabs_;
  BooleanPrefMember showing_window_title_for_vertical_tabs_;

  // True unless focus mode is mid-animation or hiding the caption buttons.
  // Gates the caption button hit-test path in `NonClientHitTest`: while false,
  // the container's visual band is reported as client area to avoid the
  // "invisible-but-clickable" effect that occurs because View hit testing is
  // bounds-based and ignores layer transforms.
  bool focus_mode_caption_revealed_ = true;

  std::unique_ptr<FocusModeRevealObserver> focus_mode_reveal_observer_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_BROWSER_FRAME_VIEW_WIN_H_
