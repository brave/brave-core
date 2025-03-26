// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/api/web_view/brave_web_view.h"

#include "base/notreached.h"
#include "ios/chrome/browser/tabs/model/tab_helper_util.h"
#include "ios/web/public/web_state.h"
#include "ios/web_view/internal/cwv_web_view_internal.h"

@interface CWVWebView ()
- (void)attachSecurityInterstitialHelpersToWebStateIfNecessary;
@end

@implementation BraveWebView

// These are shadowed CWVWebView properties
@dynamic navigationDelegate, UIDelegate;

- (void)attachSecurityInterstitialHelpersToWebStateIfNecessary {
  [super attachSecurityInterstitialHelpersToWebStateIfNecessary];
  AttachTabHelpers(self.webState);
}

- (CWVAutofillController*)autofillController {
  NOTREACHED();
  return nil;
}

- (CWVTranslationController*)translationController {
  NOTREACHED();
  return nil;
}

#pragma mark - CRWWebStateDelegate

- (void)webState:(web::WebState*)webState
    didRequestHTTPAuthForProtectionSpace:(NSURLProtectionSpace*)protectionSpace
                      proposedCredential:(NSURLCredential*)proposedCredential
                       completionHandler:(void (^)(NSString* username,
                                                   NSString* password))handler {
  SEL selector = @selector(webView:
      didRequestHTTPAuthForProtectionSpace:proposedCredential:completionHandler
                                          :);
  if ([self.navigationDelegate respondsToSelector:selector]) {
    [self.navigationDelegate webView:self
        didRequestHTTPAuthForProtectionSpace:protectionSpace
                          proposedCredential:proposedCredential
                           completionHandler:handler];
  } else {
    handler(nil, nil);
  }
}

@end
