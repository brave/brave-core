/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "ios/web_view/internal/web_view_browser_state.h"

namespace ios_web_view {

// static
WebViewBrowserState* WebViewBrowserState::FromBrowserState(
    web::BrowserState* browser_state) {
  return static_cast<WebViewBrowserState*>(browser_state);
}

WebViewBrowserState* WebViewBrowserState::GetRecordingBrowserState() {
  return static_cast<WebViewBrowserState*>(GetOriginalChromeBrowserState());
}

}  // namespace ios_web_view
