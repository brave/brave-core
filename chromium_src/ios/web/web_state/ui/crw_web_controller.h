/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_IOS_WEB_WEB_STATE_UI_CRW_WEB_CONTROLLER_H_
#define BRAVE_CHROMIUM_SRC_IOS_WEB_WEB_STATE_UI_CRW_WEB_CONTROLLER_H_

#define webViewNavigationProxy                                       \
  webViewNavigationProxy_ChromiumImpl;                               \
  @property(weak, nonatomic, readonly) id<CRWWebViewNavigationProxy> \
      webViewNavigationProxy;                                        \
  @property(weak, nonatomic, readonly) WKWebView* webView

#include "src/ios/web/web_state/ui/crw_web_controller.h"  // IWYU pragma: export

#undef webViewNavigationProxy

#endif  // BRAVE_CHROMIUM_SRC_IOS_WEB_WEB_STATE_UI_CRW_WEB_CONTROLLER_H_
