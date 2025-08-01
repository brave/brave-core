// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ios/web/navigation/crw_wk_navigation_handler.h"

#import <Foundation/Foundation.h>
#import <WebKit/WebKit.h>

#include "ios/web/common/user_agent.h"
#import "ios/web/public/web_client.h"

#include <ios/web/navigation/crw_wk_navigation_handler.mm>

// Setup a CRWWKNavigationHandler subclass so that we may integrate Brave
// features that Chrome does not support or expose access to (such as the
// requiring the use of WKWebpagePreferences)
@implementation BraveCRWWKNavigationHandler

- (void)webView:(WKWebView*)webView
    decidePolicyForNavigationAction:(WKNavigationAction*)action
                        preferences:(WKWebpagePreferences*)preferences
                    decisionHandler:(void (^)(WKNavigationActionPolicy,
                                              WKWebpagePreferences*))handler {
  auto decisionHandler =
      ^(WKNavigationActionPolicy policy, WKWebpagePreferences*) {
        // Check if we want to block JavaScript execution on the page
        // Typically this would go through `ContentSettingsType::JAVASCRIPT`,
        // but Chrome iOS does not support this yet.
        bool shouldBlockJavaScript = web::GetWebClient()->ShouldBlockJavaScript(
            static_cast<web::WebState*>(self.webStateImpl), action.request);
        if (shouldBlockJavaScript && preferences) {
          // Only ever update it to false
          preferences.allowsContentJavaScript = false;
        }

        if (@available(iOS 18, *)) {
        } else {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
          // In prior versions of iOS,
          // `WKWebpagePreferences.allowsContentJavaScript` does not work
          // correctly in all cases:
          // https://github.com/brave/brave-ios/issues/8585
          webView.configuration.preferences.javaScriptEnabled =
              preferences.allowsContentJavaScript;
#pragma clang diagnostic pop
        }

        const web::UserAgentType userAgentType =
            [self userAgentForNavigationAction:action webView:webView];
        if (userAgentType != web::UserAgentType::NONE) {
          NSString* userAgent = web::GetWebClient()->GetUserAgentForRequest(
              static_cast<web::WebState*>(self.webStateImpl), userAgentType,
              action.request);
          if (userAgent &&
              ![webView.customUserAgent isEqualToString:userAgent]) {
            webView.customUserAgent = userAgent;
          }
        }

        if (policy == WKNavigationActionPolicyAllow) {
          // Check if we want to explicitly block universal links
          bool forceBlockUniversalLinks =
              web::GetWebClient()->ShouldBlockUniversalLinks(
                  static_cast<web::WebState*>(self.webStateImpl),
                  action.request);
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
