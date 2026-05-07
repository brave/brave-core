/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_BROWSER_FRAME_VIEW_WIN_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_BROWSER_FRAME_VIEW_WIN_H_

#include <memory>

#include "brave/browser/ui/focus_mode/focus_mode_controller.h"
#include "chrome/browser/ui/views/frame/browser_frame_view_win.h"
#include "components/prefs/pref_member.h"
#include "ui/base/metadata/metadata_header_macros.h"

class BraveWindowFrameGraphic;

class BraveBrowserFrameViewWin : public BrowserFrameViewWin,
                                 public FocusModeController::Observer {
  METADATA_HEADER(BraveBrowserFrameViewWin, BrowserFrameViewWin)
 public:
  BraveBrowserFrameViewWin(BrowserWidget* browser_widget,
                           BrowserView* browser_view);
  ~BraveBrowserFrameViewWin() override;

  BraveBrowserFrameViewWin(const BraveBrowserFrameViewWin&) = delete;
  BraveBrowserFrameViewWin& operator=(const BraveBrowserFrameViewWin&) = delete;

  bool ShouldCaptionButtonsBeDrawnOverToolbar() const;

 private:
  void OnVerticalTabsPrefsChanged();
  void OnTopOverlayRevealFractionChanged(double reveal_fraction);

  // BraveBrowserFrameViewWin overrides:
  void OnPaint(gfx::Canvas* canvas) override;
  int GetTopInset(bool restored) const override;
  int NonClientHitTest(const gfx::Point& point) override;
  bool ShouldShowWindowTitle(TitlebarType type) const override;
  void LayoutCaptionButtons() override;
  int FrameTopBorderThickness(bool restored) const override;

  // FocusModeController::Observer:
  void OnFocusModeToggled(bool enabled) override;

  std::unique_ptr<BraveWindowFrameGraphic> frame_graphic_;

  BooleanPrefMember using_vertical_tabs_;
  BooleanPrefMember showing_window_title_for_vertical_tabs_;

  base::ScopedObservation<FocusModeController, FocusModeController::Observer>
      focus_mode_observation_{this};
  base::CallbackListSubscription overlay_reveal_subscription_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_BROWSER_FRAME_VIEW_WIN_H_
