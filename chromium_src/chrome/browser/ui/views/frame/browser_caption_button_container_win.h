/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_BROWSER_CAPTION_BUTTON_CONTAINER_WIN_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_BROWSER_CAPTION_BUTTON_CONTAINER_WIN_H_

#include "ui/base/metadata/metadata_header_macros.h"

#define BrowserCaptionButtonContainer BrowserCaptionButtonContainer_ChromiumImpl
#define OnWindowControlsOverlayEnabledChanged     \
  OnWindowControlsOverlayEnabledChanged_Unused(); \
  friend class BraveBrowserFrameViewWin;          \
  void OnWindowControlsOverlayEnabledChanged

#include "src/chrome/browser/ui/views/frame/browser_caption_button_container_win.h"  // IWYU pragma: export

#undef OnWindowControlsOverlayEnabledChanged
#undef BrowserCaptionButtonContainer

class BrowserCaptionButtonContainer
    : public BrowserCaptionButtonContainer_ChromiumImpl {
  METADATA_HEADER(BrowserCaptionButtonContainer,
                  BrowserCaptionButtonContainer_ChromiumImpl)
 public:
  explicit BrowserCaptionButtonContainer(BrowserFrameViewWin* frame_view);
  ~BrowserCaptionButtonContainer() override;

 private:
  const raw_ptr<BrowserFrameViewWin> frame_view_;
};

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_BROWSER_CAPTION_BUTTON_CONTAINER_WIN_H_
