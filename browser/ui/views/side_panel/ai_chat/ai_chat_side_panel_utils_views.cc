// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/side_panel/ai_chat/ai_chat_side_panel_utils.h"

#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/views/frame/browser_view.h"

namespace ai_chat {

Browser* GetBrowserForWebContents(content::WebContents* web_contents) {
  auto* browser_window =
      BrowserWindow::FindBrowserWindowWithWebContents(web_contents);
  auto* browser_view = static_cast<BrowserView*>(browser_window);
  CHECK(browser_view);
  return browser_view->browser();
}

}  // namespace ai_chat
