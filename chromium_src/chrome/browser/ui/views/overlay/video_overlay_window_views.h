/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_OVERLAY_VIDEO_OVERLAY_WINDOW_VIEWS_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_OVERLAY_VIDEO_OVERLAY_WINDOW_VIEWS_H_

class BraveBackToTabLabelButton;

#define SetUpViews                           \
  SetUpViews_Unused();                       \
  friend class BraveVideoOverlayWindowViews; \
  virtual void SetUpViews

#define OnUpdateControlsBounds virtual OnUpdateControlsBounds
#define BackToTabLabelButton BraveBackToTabLabelButton
#define ControlsHitTestContainsPoint virtual ControlsHitTestContainsPoint

#include "src/chrome/browser/ui/views/overlay/video_overlay_window_views.h"  // IWYU pragma: export

#undef ControlsHitTestContainsPoint
#undef BackToTabLabelButton
#undef OnUpdateControlsBounds
#undef SetUpViews

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_OVERLAY_VIDEO_OVERLAY_WINDOW_VIEWS_H_
