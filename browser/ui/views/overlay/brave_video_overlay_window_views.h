/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_OVERLAY_BRAVE_VIDEO_OVERLAY_WINDOW_VIEWS_H_
#define BRAVE_BROWSER_UI_VIEWS_OVERLAY_BRAVE_VIDEO_OVERLAY_WINDOW_VIEWS_H_

#include "chrome/browser/ui/views/overlay/video_overlay_window_views.h"

class BraveVideoOverlayWindowViews : public VideoOverlayWindowViews {
 public:
  explicit BraveVideoOverlayWindowViews(
      content::VideoPictureInPictureWindowController* controller);
  ~BraveVideoOverlayWindowViews() override = default;

  // VideoOverlayWindowViews:
  void SetUpViews() override;
  void OnUpdateControlsBounds() override;

  void UpdateControlIcons();
};

#endif  // BRAVE_BROWSER_UI_VIEWS_OVERLAY_BRAVE_VIDEO_OVERLAY_WINDOW_VIEWS_H_
