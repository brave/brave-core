/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_IOS_WEB_VIEW_INTERNAL_WEB_VIEW_BROWSER_STATE_H_
#define BRAVE_CHROMIUM_SRC_IOS_WEB_VIEW_INTERNAL_WEB_VIEW_BROWSER_STATE_H_

#include "ios/chrome/browser/shared/model/browser_state/chrome_browser_state.h"

namespace ios_web_view {

class WebViewBrowserState : public ChromeBrowserState {
 public:
  // Converts from web::BrowserState to WebViewBrowserState.
  static WebViewBrowserState* FromBrowserState(
      web::BrowserState* browser_state);

  WebViewBrowserState* GetRecordingBrowserState();
};

}  // namespace ios_web_view

#endif
