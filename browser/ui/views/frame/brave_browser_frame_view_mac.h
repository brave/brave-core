/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_BROWSER_FRAME_VIEW_MAC_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_BROWSER_FRAME_VIEW_MAC_H_

#include <memory>

#include "base/callback_list.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observation.h"
#include "brave/browser/ui/focus_mode/focus_mode_controller.h"
#include "chrome/browser/ui/views/frame/browser_frame_view_mac.h"

class BraveWindowFrameGraphic;

class BraveBrowserFrameViewMac : public BrowserFrameViewMac,
                                 public FocusModeController::Observer {
 public:
  BraveBrowserFrameViewMac(BrowserWidget* browser_widget,
                           BrowserView* browser_view);
  ~BraveBrowserFrameViewMac() override;

  BraveBrowserFrameViewMac(const BraveBrowserFrameViewMac&) = delete;
  BraveBrowserFrameViewMac& operator=(const BraveBrowserFrameViewMac&) = delete;
  gfx::Size GetMinimumSize() const override;

  // BrowserFrameView:
  void OnFullscreenStateChanged() override;

  // FocusModeController::Observer:
  void OnFocusModeToggled(bool enabled) override;

 private:
  class ScopedFocusModeDisable;

  bool ShouldShowWindowTitleForVerticalTabs() const;
  void UpdateWindowTitleVisibility();
  void UpdateWindowTitleAndControls();
  void UpdateWindowTitleColor();
  void UpdateWindowControlsOpacity();
  void OnTopOverlayRevealFractionChanged(double reveal_fraction);
  void ResetWindowControlsPosition();

  // BrowserFrameViewMac overrides:
  gfx::Rect GetBoundsForClientView() const override;
  void OnPaint(gfx::Canvas* canvas) override;
  int GetTopInset(bool restored) const override;
  BoundsAndMargins GetCaptionButtonBounds() const override;
  int NonClientHitTest(const gfx::Point& point) override;
  void OnThemeChanged() override;
  bool ShouldHideTopUIInFullscreen() const override;

  std::unique_ptr<BraveWindowFrameGraphic> frame_graphic_;

  BooleanPrefMember show_vertical_tabs_;
  BooleanPrefMember show_title_bar_on_vertical_tabs_;
  BooleanPrefMember compact_horizontal_tabs_;

  std::unique_ptr<ScopedFocusModeDisable> scoped_focus_mode_disable_;
  base::ScopedObservation<FocusModeController, FocusModeController::Observer>
      focus_mode_observation_{this};
  base::CallbackListSubscription overlay_reveal_subscription_;
  base::WeakPtrFactory<BraveBrowserFrameViewMac> weak_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_BROWSER_FRAME_VIEW_MAC_H_
