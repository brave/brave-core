/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/speedreader/reader_mode_panel_view.h"

#include "brave/components/constants/webui_url_constants.h"
#include "ui/views/controls/webview/webview.h"
#include "url/gurl.h"

ReaderModePanelView::ReaderModePanelView(
    content::BrowserContext* browser_context)
    : views::WebView(browser_context) {
  LoadInitialURL(GURL(kSpeedreaderPanelURL));
  SetBounds(0, 0, 0, 40);
}

ReaderModePanelView::~ReaderModePanelView() = default;

bool ReaderModePanelView::HandleContextMenu(
    content::RenderFrameHost& render_frame_host,
    const content::ContextMenuParams& params) {
  // Ignore context menu.
  return true;
}
