/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_WIN_CAPTION_LAYOUT_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_WIN_CAPTION_LAYOUT_H_

namespace brave {

// While active, BrowserFrameViewWin caption layout uses the same
// LayoutConstant::kTabstripToolbarOverlap as tab row / WebAppFrameToolbar
// geometry instead of tabs::GetHorizontalTabControlsDelta(). See
// GetLayoutConstantForBraveWindowControls in
// chromium_src/chrome/browser/ui/views/frame/browser_frame_view_win.cc.
class ScopedWinCaptionLayoutUsesGeometryTabstripOverlap {
 public:
  explicit ScopedWinCaptionLayoutUsesGeometryTabstripOverlap(bool enable);
  ~ScopedWinCaptionLayoutUsesGeometryTabstripOverlap();

  ScopedWinCaptionLayoutUsesGeometryTabstripOverlap(
      const ScopedWinCaptionLayoutUsesGeometryTabstripOverlap&) = delete;
  ScopedWinCaptionLayoutUsesGeometryTabstripOverlap& operator=(
      const ScopedWinCaptionLayoutUsesGeometryTabstripOverlap&) = delete;

 private:
  bool enabled_ = false;
};

}  // namespace brave

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_WIN_CAPTION_LAYOUT_H_
