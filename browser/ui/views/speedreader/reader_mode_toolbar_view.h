/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SPEEDREADER_READER_MODE_TOOLBAR_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_SPEEDREADER_READER_MODE_TOOLBAR_VIEW_H_

#include <memory>

#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/controls/webview/webview.h"

namespace content {
class BrowserContext;
}

class ReaderModeToolbarView : public views::View {
  METADATA_HEADER(ReaderModeToolbarView, views::View)
 public:
  explicit ReaderModeToolbarView(content::BrowserContext* browser_context);
  ~ReaderModeToolbarView() override;

  ReaderModeToolbarView(const ReaderModeToolbarView&) = delete;
  ReaderModeToolbarView(ReaderModeToolbarView&&) = delete;
  ReaderModeToolbarView& operator=(const ReaderModeToolbarView&) = delete;
  ReaderModeToolbarView& operator=(ReaderModeToolbarView&&) = delete;

  content::WebContents* GetWebContentsForTesting();

  views::View* toolbar() const { return toolbar_.get(); }

 private:
  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void OnBoundsChanged(const gfx::Rect& previous_bounds) override;

  std::unique_ptr<views::WebView> toolbar_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SPEEDREADER_READER_MODE_TOOLBAR_VIEW_H_
