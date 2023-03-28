/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_DOWNLOAD_BUBBLE_DOWNLOAD_TOOLBAR_BUTTON_VIEW_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_DOWNLOAD_BUBBLE_DOWNLOAD_TOOLBAR_BUTTON_VIEW_H_

#include "chrome/browser/download/bubble/download_display_controller.h"

#define DownloadToolbarButtonView DownloadToolbarButtonViewChromium
#define GetIconColor                                        \
  UnUsed() {                                                \
    return SK_ColorTRANSPARENT;                             \
  }                                                         \
                                                            \
 protected:                                                 \
  DownloadDisplayController::IconInfo GetIconInfo() const { \
    return controller_->GetIconInfo();                      \
  }                                                         \
                                                            \
 public:                                                    \
  virtual SkColor GetIconColor

#include "src/chrome/browser/ui/views/download/bubble/download_toolbar_button_view.h"  // IWYU pragma: export

#undef GetIconColor
#undef DownloadToolbarButtonView

class DownloadToolbarButtonView : public DownloadToolbarButtonViewChromium {
 public:
  using DownloadToolbarButtonViewChromium::DownloadToolbarButtonViewChromium;

  // DownloadToolbarButtonViewChromium overrides:
  SkColor GetIconColor() const override;
};

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_DOWNLOAD_BUBBLE_DOWNLOAD_TOOLBAR_BUTTON_VIEW_H_
