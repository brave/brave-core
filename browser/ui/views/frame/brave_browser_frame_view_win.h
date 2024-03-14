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

class BraveBrowserFrameViewWin : public BrowserFrameViewWin {
 public:
  BraveBrowserFrameViewWin(BrowserFrame* frame, BrowserView* browser_view);
  ~BraveBrowserFrameViewWin() override;

  BraveBrowserFrameViewWin(const BraveBrowserFrameViewWin&) = delete;
  BraveBrowserFrameViewWin& operator=(const BraveBrowserFrameViewWin&) = delete;

 private:
  void OnVerticalTabsPrefsChanged();

  // BraveBrowserFrameViewWin overrides:
  void OnPaint(gfx::Canvas* canvas) override;
  int GetTopInset(bool restored) const override;
  int NonClientHitTest(const gfx::Point& point) override;

  // BrowserNonClientFrameView overrides:
  void OnFullscreenStateChanged() override;

  std::unique_ptr<BraveWindowFrameGraphic> frame_graphic_;

  BooleanPrefMember using_vertical_tabs_;
  BooleanPrefMember showing_window_title_for_vertical_tabs_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_BROWSER_FRAME_VIEW_WIN_H_
