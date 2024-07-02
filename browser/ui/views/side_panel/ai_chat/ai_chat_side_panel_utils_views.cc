// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/side_panel/ai_chat/ai_chat_side_panel_utils.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/side_panel/side_panel_ui.h"

namespace ai_chat {

Browser* GetBrowserForWebContents(content::WebContents* web_contents) {
  if (!web_contents) {
    return nullptr;
  }

  auto* browser_window =
      BrowserWindow::FindBrowserWindowWithWebContents(web_contents);
  auto* browser_view = static_cast<BrowserView*>(browser_window);
  if (!browser_view) {
    return nullptr;
  }

  return browser_view->browser();
}

void ClosePanel(content::WebContents* web_contents) {
  if (!web_contents) {
    return;
  }

  Browser* browser = GetBrowserForWebContents(web_contents);
  if (!browser) {
    return;
  }

  if (SidePanelUI* ui = SidePanelUI::GetSidePanelUIForBrowser(browser)) {
    ui->Close();
  }
}

}  // namespace ai_chat
