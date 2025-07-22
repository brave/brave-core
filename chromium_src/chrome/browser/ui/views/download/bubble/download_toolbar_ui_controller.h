/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_DOWNLOAD_BUBBLE_DOWNLOAD_TOOLBAR_UI_CONTROLLER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_DOWNLOAD_BUBBLE_DOWNLOAD_TOOLBAR_UI_CONTROLLER_H_

class DownloadToolbarUIController;
using DownloadToolbarUIController_BraveImpl = DownloadToolbarUIController;

#define DownloadToolbarUIController DownloadToolbarUIController_ChromiumImpl

#define CreateBubbleDialogDelegate              \
  Unused();                                     \
  friend DownloadToolbarUIController_BraveImpl; \
  void CreateBubbleDialogDelegate

// Override to show warning icon in the toolbar when there's an insecure
// download in progress.
#define UpdateIcon virtual UpdateIcon

#include <chrome/browser/ui/views/download/bubble/download_toolbar_ui_controller.h>  // IWYU pragma: export
#undef UpdateIcon
#undef DownloadToolbarUIController
#undef CreateBubbleDialogDelegate

class DownloadToolbarUIController
    : public DownloadToolbarUIController_ChromiumImpl {
 public:
  using DownloadToolbarUIController_ChromiumImpl::
      DownloadToolbarUIController_ChromiumImpl;

  void UpdateIcon() override;

 private:
  bool HasInsecureDownloads();
};

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_DOWNLOAD_BUBBLE_DOWNLOAD_TOOLBAR_UI_CONTROLLER_H_
