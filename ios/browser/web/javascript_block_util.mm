// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#import <WebKit/WebKit.h>

#import "brave/ios/web_view/public/chrome_web_view.h"
#include "ios/chrome/browser/shared/model/application_context/application_context.h"
#include "ios/web/common/url_scheme_util.h"
#include "ios/web/public/web_state.h"
#import "ios/web_view/internal/cwv_web_view_internal.h"
#import "ios/web_view/public/cwv_navigation_delegate.h"

namespace brave {

bool ShouldBlockJavaScript(web::WebState* webState, NSURLRequest* request) {
  if (!web::UrlHasWebScheme(request.URL)) {
    return false;
  }
  BraveWebView* webView =
      static_cast<BraveWebView*>([BraveWebView webViewForWebState:webState]);
  id<BraveWebViewNavigationDelegate> navigationDelegate =
      webView.navigationDelegate;

  if ([navigationDelegate respondsToSelector:@selector
                          (webView:shouldBlockJavaScriptForRequest:)]) {
    return [navigationDelegate webView:webView
        shouldBlockJavaScriptForRequest:request];
  }
  return false;
}

}  // namespace brave
