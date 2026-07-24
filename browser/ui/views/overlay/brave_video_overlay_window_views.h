/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_OVERLAY_BRAVE_VIDEO_OVERLAY_WINDOW_VIEWS_H_
#define BRAVE_BROWSER_UI_VIEWS_OVERLAY_BRAVE_VIDEO_OVERLAY_WINDOW_VIEWS_H_

#include "base/timer/elapsed_timer.h"
#include "base/timer/timer.h"
#include "build/build_config.h"
#include "chrome/browser/ui/views/overlay/video_overlay_window_views.h"
#include "services/media_session/public/cpp/media_position.h"
#include "ui/views/controls/slider.h"

// This view is a overlay window over PIP window to show controls on top of the
// PIP window. We're restyling upstream's controls and adding some
// functionalities.
class BraveVideoOverlayWindowViews : public VideoOverlayWindowViews {
 public:
  explicit BraveVideoOverlayWindowViews(
      content::VideoPictureInPictureWindowController* controller);
  ~BraveVideoOverlayWindowViews() override;

#if BUILDFLAG(IS_LINUX)
  // Sets WM_CLASS (X11) and the app id (Wayland) on the picture-in-picture
  // widget's init params so Linux window managers can match the window to the
  // browser's (or source web app's) .desktop entry. Without them the window
  // has none. Called from VideoOverlayWindowViews::Create() via chromium_src.
  static void SetLinuxWMClass(
      views::Widget::InitParams& params,
      content::VideoPictureInPictureWindowController* controller);
#endif  // BUILDFLAG(IS_LINUX)

  // VideoOverlayWindowViews:
  void SetUpViews() override;
  void OnUpdateControlsBounds() override;
  void OnKeyEvent(ui::KeyEvent* event) override;

 private:
  void UpdateControlIcons();
  void LayoutTopControls();
  void LayoutCenterControls();
};

#endif  // BRAVE_BROWSER_UI_VIEWS_OVERLAY_BRAVE_VIDEO_OVERLAY_WINDOW_VIEWS_H_
