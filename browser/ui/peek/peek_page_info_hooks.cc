/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Hooks called from the chromium_src override of ChromePageInfoDelegate. They
// are declared there, so no header is needed.

#include "brave/browser/ui/peek/public/peek_controller.h"
#include "content/public/browser/web_contents.h"

namespace peek {

void ShowCookiesDialogInTab(content::WebContents* web_contents) {
  if (auto* controller = PeekController::FromPeekContents(web_contents)) {
    controller->ShowCookiesDialogInTab();
  }
}

}  // namespace peek
