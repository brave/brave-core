/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BROWSER_UI_VIEWS_SPEEDREADER_READER_MODE_PANEL_VIEW_H_
#define BROWSER_UI_VIEWS_SPEEDREADER_READER_MODE_PANEL_VIEW_H_

#include "content/public/browser/browser_context.h"
#include "content/public/browser/context_menu_params.h"
#include "content/public/browser/render_frame_host.h"
#include "ui/views/controls/webview/webview.h"

class ReaderModePanelView : public views::WebView {
 public:
  explicit ReaderModePanelView(content::BrowserContext* browser_context);
  ~ReaderModePanelView() override;

 private:
  // WebView:
  bool HandleContextMenu(content::RenderFrameHost& render_frame_host,
                         const content::ContextMenuParams& params) override;
};

#endif  // BROWSER_UI_VIEWS_SPEEDREADER_READER_MODE_PANEL_VIEW_H_
