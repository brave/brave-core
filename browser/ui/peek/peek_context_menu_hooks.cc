/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Hooks called from the chromium_src override of RenderViewContextMenu. They
// are declared there, so no header is needed.

#include "brave/browser/ui/peek/peek_utils.h"
#include "brave/browser/ui/peek/public/peek_controller.h"
#include "content/public/browser/page_navigator.h"
#include "url/gurl.h"

namespace peek {

bool CanOpenLinkInPeek(BrowserWindowInterface* browser, const GURL& url) {
  return IsPeekableURL(url) && PeekController::From(browser);
}

void OpenLinkInPeek(BrowserWindowInterface* browser,
                    const content::OpenURLParams& params) {
  if (auto* controller = PeekController::From(browser)) {
    controller->Open(params);
  }
}

}  // namespace peek
