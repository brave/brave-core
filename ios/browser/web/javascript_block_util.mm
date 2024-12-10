// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#import <WebKit/WebKit.h>

#include "ios/chrome/browser/shared/model/application_context/application_context.h"
#include "ios/web/public/web_state.h"
#import "ios/web_view/internal/cwv_web_view_internal.h"
#import "ios/web_view/public/cwv_navigation_delegate.h"

namespace brave {

void ShouldBlockJavaScript(web::WebState* webState,
                           NSURLRequest* request,
                           WKWebpagePreferences* preferences) {
  CWVWebView* webView = [CWVWebView webViewForWebState:webState];
  id<CWVNavigationDelegate> navigationDelegate = webView.navigationDelegate;

  if ([navigationDelegate respondsToSelector:@selector
                          (webView:shouldBlockJavaScriptForRequest:)]) {
    BOOL shouldBlockJavaScript = [navigationDelegate webView:webView
                             shouldBlockJavaScriptForRequest:request];
    if (shouldBlockJavaScript && preferences) {
      // Only ever update it to false
      preferences.allowsContentJavaScript = false;
    }
  }
}

}  // namespace brave
