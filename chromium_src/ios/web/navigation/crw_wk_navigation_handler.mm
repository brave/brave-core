// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ios/web/navigation/crw_wk_navigation_handler.h"

#import <Foundation/Foundation.h>
#import <WebKit/WebKit.h>

#include "src/ios/web/navigation/crw_wk_navigation_handler.mm"

@implementation BraveCRWWKNavigationHandler

- (void)webView:(WKWebView*)webView
    decidePolicyForNavigationAction:(WKNavigationAction*)action
                        preferences:(WKWebpagePreferences*)preferences
                    decisionHandler:(void (^)(WKNavigationActionPolicy,
                                              WKWebpagePreferences*))handler {
  auto decisionHandler =
      ^(WKNavigationActionPolicy policy, WKWebpagePreferences*) {
        bool shouldBlockJavaScript = brave::ShouldBlockJavaScript(
            static_cast<web::WebState*>(self.webStateImpl), action.request);
        if (shouldBlockJavaScript && preferences) {
          // Only ever update it to false
          preferences.allowsContentJavaScript = false;
        }
        if (policy == WKNavigationActionPolicyAllow) {
          // Check if we want to explicitly block universal links
          bool forceBlockUniversalLinks = brave::ShouldBlockUniversalLinks(
              static_cast<web::WebState*>(self.webStateImpl), action.request);
          if (forceBlockUniversalLinks) {
            handler(web::kNavigationActionPolicyAllowAndBlockUniversalLinks,
                    preferences);
            return;
          }
        }
        handler(policy, preferences);
      };
  [super webView:webView
      decidePolicyForNavigationAction:action
                          preferences:preferences
                      decisionHandler:decisionHandler];
}

@end
