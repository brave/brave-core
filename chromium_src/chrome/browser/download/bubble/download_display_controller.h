/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_DOWNLOAD_BUBBLE_DOWNLOAD_DISPLAY_CONTROLLER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_DOWNLOAD_BUBBLE_DOWNLOAD_DISPLAY_CONTROLLER_H_

#define DownloadDisplayController DownloadDisplayControllerChromium

// To make private methods accessible from subclass.
#define DownloadDisplayControllerTest \
  UnUsed;                             \
                                      \
 protected:                           \
  friend class DownloadDisplayControllerTest

#define UpdateToolbarButtonState virtual UpdateToolbarButtonState

#include "src/chrome/browser/download/bubble/download_display_controller.h"  // IWYU pragma: export

#undef UpdateToolbarButtonState
#undef DownloadDisplayControllerTest
#undef DownloadDisplayController

class DownloadDisplayController : public DownloadDisplayControllerChromium {
 public:
  using DownloadDisplayControllerChromium::DownloadDisplayControllerChromium;

 private:
  void UpdateToolbarButtonState(
      const DownloadBubbleDisplayInfo& info,
      const DownloadDisplay::ProgressInfo& progress_info) override;
};

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_DOWNLOAD_BUBBLE_DOWNLOAD_DISPLAY_CONTROLLER_H_
