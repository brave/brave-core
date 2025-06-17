/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_DOWNLOAD_BUBBLE_DOWNLOAD_TOOLBAR_UI_CONTROLLER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_DOWNLOAD_BUBBLE_DOWNLOAD_TOOLBAR_UI_CONTROLLER_H_

#define DownloadToolbarUIController DownloadToolbarUIController_ChromiumImpl

// Apply active color only when download is completed and user doesn't
// interact with this button.
#define UpdateIconDormant                                                      \
  UpdateIconDormant();                                                         \
                                                                               \
 protected:                                                                    \
  virtual SkColor GetIconColor(bool is_dormant,                                \
                               DownloadDisplay::IconActive active,             \
                               const ui::ColorProvider* color_provider) const; \
  void UnUsed

// Override to show warning icon in the toolbar when there's an insecure
// download in progress.
#define UpdateIcon virtual UpdateIcon

#include <chrome/browser/ui/views/download/bubble/download_toolbar_ui_controller.h>  // IWYU pragma: export
#undef UpdateIcon
#undef UpdateIconDormant
#undef DownloadToolbarUIController

class DownloadToolbarUIController
    : public DownloadToolbarUIController_ChromiumImpl {
 public:
  using DownloadToolbarUIController_ChromiumImpl::
      DownloadToolbarUIController_ChromiumImpl;

  SkColor GetIconColor(bool is_dormant,
                       DownloadDisplay::IconActive active,
                       const ui::ColorProvider* color_provider) const override;

  void UpdateIcon() override;

 private:
  bool HasInsecureDownloads();
};

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_DOWNLOAD_BUBBLE_DOWNLOAD_TOOLBAR_UI_CONTROLLER_H_
