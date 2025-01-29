// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#import "brave/ios/web_view/public/chrome_web_view.h"
#include "ios/chrome/browser/shared/model/application_context/application_context.h"
#include "ios/web/public/web_state.h"
#import "ios/web_view/internal/cwv_web_view_internal.h"

namespace brave {

void ShouldBlockUniversalLinks(web::WebState* webState,
                               NSURLRequest* request,
                               bool* forceBlockUniversalLinks) {
  BraveWebView* webView =
      static_cast<BraveWebView*>([BraveWebView webViewForWebState:webState]);
  id<BraveWebViewNavigationDelegate> navigationDelegate =
      webView.navigationDelegate;

  if ([navigationDelegate respondsToSelector:@selector
                          (webView:shouldBlockUniversalLinksForRequest:)]) {
    BOOL shouldBlockUniversalLinks = [navigationDelegate webView:webView
                             shouldBlockUniversalLinksForRequest:request];
    if (forceBlockUniversalLinks && shouldBlockUniversalLinks) {
      // Only ever update it to true
      *forceBlockUniversalLinks = shouldBlockUniversalLinks;
    }
  }
}

}  // namespace brave
