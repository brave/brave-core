/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SPEEDREADER_READER_MODE_PANEL_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_SPEEDREADER_READER_MODE_PANEL_VIEW_H_

#include <memory>

#include "content/public/browser/browser_context.h"
#include "ui/views/controls/webview/webview.h"

class ReaderModePanelView : public views::View {
 public:
  explicit ReaderModePanelView(content::BrowserContext* browser_context);
  ~ReaderModePanelView() override;

 private:
  gfx::Size CalculatePreferredSize() const override;
  void OnBoundsChanged(const gfx::Rect& previous_bounds) override;

  std::unique_ptr<views::WebView> toolbar_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SPEEDREADER_READER_MODE_PANEL_VIEW_H_
