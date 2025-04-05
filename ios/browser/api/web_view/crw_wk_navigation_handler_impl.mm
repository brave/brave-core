// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#import <Foundation/Foundation.h>

#import "brave/ios/browser/api/web_view/brave_web_view.h"
#include "brave/ios/web_view/public/cwv_web_view_extras.h"
#include "ios/web/common/url_scheme_util.h"
#include "ios/web/common/user_agent.h"
#include "ios/web/public/web_state.h"
#import "ios/web_view/internal/cwv_web_view_internal.h"

namespace brave {

bool ShouldBlockJavaScript(web::WebState* webState, NSURLRequest* request) {
  if (!web::UrlHasWebScheme(request.URL)) {
    return false;
  }
  BraveWebView* webView =
      static_cast<BraveWebView*>([BraveWebView webViewForWebState:webState]);
  if (!webView) {
    return false;
  }
  id<BraveWebViewNavigationDelegate> navigationDelegate =
      webView.navigationDelegate;

  if ([navigationDelegate respondsToSelector:@selector
                          (webView:shouldBlockJavaScriptForRequest:)]) {
    return [navigationDelegate webView:webView
        shouldBlockJavaScriptForRequest:request];
  }
  return false;
}

bool ShouldBlockUniversalLinks(web::WebState* webState, NSURLRequest* request) {
  BraveWebView* webView =
      static_cast<BraveWebView*>([BraveWebView webViewForWebState:webState]);
  if (!webView) {
    return false;
  }
  id<BraveWebViewNavigationDelegate> navigationDelegate =
      webView.navigationDelegate;

  if ([navigationDelegate respondsToSelector:@selector
                          (webView:shouldBlockUniversalLinksForRequest:)]) {
    return [navigationDelegate webView:webView
        shouldBlockUniversalLinksForRequest:request];
  }
  return false;
}

NSString* GetUserAgentForRequest(web::WebState* webState,
                                 web::UserAgentType userAgentType,
                                 NSURLRequest* request) {
  BraveWebView* webView =
      static_cast<BraveWebView*>([BraveWebView webViewForWebState:webState]);
  if (!webView) {
    return nil;
  }

  id<BraveWebViewNavigationDelegate> navigationDelegate =
      webView.navigationDelegate;
  if ([navigationDelegate respondsToSelector:@selector
                          (webView:userAgentForUserAgentType:request:)]) {
    return [navigationDelegate
                          webView:webView
        userAgentForUserAgentType:static_cast<CWVUserAgentType>(userAgentType)
                          request:request];
  }
  return nil;
}

}  // namespace brave
