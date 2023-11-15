/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_BROWSER_NON_CLIENT_FRAME_VIEW_MAC_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_BROWSER_NON_CLIENT_FRAME_VIEW_MAC_H_

#include <memory>

#include "chrome/browser/ui/views/frame/browser_non_client_frame_view_mac.h"

class BraveWindowFrameGraphic;

class BraveBrowserNonClientFrameViewMac : public BrowserNonClientFrameViewMac {
 public:
  BraveBrowserNonClientFrameViewMac(BrowserFrame* frame,
                                    BrowserView* browser_view);
  ~BraveBrowserNonClientFrameViewMac() override;

  BraveBrowserNonClientFrameViewMac(
      const BraveBrowserNonClientFrameViewMac&) = delete;
  BraveBrowserNonClientFrameViewMac& operator=(
      const BraveBrowserNonClientFrameViewMac&) = delete;
  gfx::Size GetMinimumSize() const override;

 private:
  bool ShouldShowWindowTitleForVerticalTabs() const;
  void UpdateWindowTitleVisibility();
  void UpdateWindowTitleAndControls();
  void UpdateWindowTitleColor();

  // BrowserNonClientFrameViewMac overrides:
  void OnPaint(gfx::Canvas* canvas) override;
  int GetTopInset(bool restored) const override;
  int NonClientHitTest(const gfx::Point& point) override;
  void OnThemeChanged() override;

  std::unique_ptr<BraveWindowFrameGraphic> frame_graphic_;

  BooleanPrefMember show_vertical_tabs_;
  BooleanPrefMember show_title_bar_on_vertical_tabs_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_BROWSER_NON_CLIENT_FRAME_VIEW_MAC_H_
