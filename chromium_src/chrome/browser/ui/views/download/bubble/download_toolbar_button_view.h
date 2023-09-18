/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_DOWNLOAD_BUBBLE_DOWNLOAD_TOOLBAR_BUTTON_VIEW_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_DOWNLOAD_BUBBLE_DOWNLOAD_TOOLBAR_BUTTON_VIEW_H_

#include "chrome/browser/download/bubble/download_bubble_ui_controller.h"
#include "chrome/browser/download/bubble/download_display_controller.h"
#include "chrome/browser/download/download_ui_model.h"
#include "chrome/browser/ui/views/toolbar/toolbar_button.h"

#define DownloadToolbarButtonView DownloadToolbarButtonViewChromium
#define PaintButtonContents                                 \
  PaintButtonContents_UnUsed() {}                           \
                                                            \
 protected:                                                 \
  void PaintButtonContents

#define GetIconColor virtual GetIconColor

#include "src/chrome/browser/ui/views/download/bubble/download_toolbar_button_view.h"  // IWYU pragma: export

#undef GetIconColor
#undef PaintButtonContents
#undef DownloadToolbarButtonView

class DownloadToolbarButtonView : public DownloadToolbarButtonViewChromium {
 public:
  using DownloadToolbarButtonViewChromium::DownloadToolbarButtonViewChromium;

  // DownloadToolbarButtonViewChromium overrides:
  void PaintButtonContents(gfx::Canvas* canvas) override;
  void UpdateIcon() override;
  SkColor GetIconColor() const override;

 private:
  bool HasInsecureDownloads();
};

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_DOWNLOAD_BUBBLE_DOWNLOAD_TOOLBAR_BUTTON_VIEW_H_
