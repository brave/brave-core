/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_BROWSER_CAPTION_BUTTON_CONTAINER_WIN_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_BROWSER_CAPTION_BUTTON_CONTAINER_WIN_H_

#define OnWindowControlsOverlayEnabledChanged     \
  OnWindowControlsOverlayEnabledChanged_Unused(); \
  friend class BraveBrowserFrameViewWin;          \
  void OnWindowControlsOverlayEnabledChanged

#include <chrome/browser/ui/views/frame/browser_caption_button_container_win.h>  // IWYU pragma: export

#undef OnWindowControlsOverlayEnabledChanged

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_BROWSER_CAPTION_BUTTON_CONTAINER_WIN_H_
