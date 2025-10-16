/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "ios/web/public/browser_state.h"
#include "ios/web/public/web_state.h"
#include "ios/web_view/internal/autofill/cwv_autofill_controller_internal.h"
#include "ios/web_view/internal/passwords/web_view_password_manager_client.h"

// We will never call to the actual methods that create the //ios/web_view
// versions of these clients since we want to use Chrome's, however we do still
// want to use CWVAutofillController with WebViewPasswordManagerClient as the
// the bridge, so instead we will call the explicit constructor on
// WebViewPasswordManagerClient directly in our BraveWebView (which uses
// Chrome's factories) and pass that along to CWVAutofillController.
//
// This empty class will only be used for compilation since typically the
// Create method takes a WebViewBrowserState which we've gotten rid of
namespace ios_web_view {
class UnusedWebViewPasswordManagerClient {
 public:
  static std::unique_ptr<WebViewPasswordManagerClient> Create(
      web::WebState* web_state,
      web::BrowserState* browser_state) {
    return nullptr;
  }
};
}  // namespace ios_web_view

#define WebViewPasswordManagerClient UnusedWebViewPasswordManagerClient
#include <ios/web_view/internal/cwv_web_view.mm>
#undef WebViewPasswordManagerClient

@implementation CWVWebView (Internal)

- (web::WebState*)webState {
  return _webState.get();
}

+ (BOOL)_isRestoreDataValid:(NSData*)data {
  NSError* error = nil;
  NSKeyedUnarchiver* coder =
      [[NSKeyedUnarchiver alloc] initForReadingFromData:data error:&error];
  if (error) {
    return NO;
  }
  coder.requiresSecureCoding = NO;
  CWVWebViewProtobufStorage* cachedProtobufStorage =
      base::apple::ObjCCastStrict<CWVWebViewProtobufStorage>(
          [coder decodeObjectForKey:kProtobufStorageKey]);
  return cachedProtobufStorage != nil;
}

+ (id)objectFromValue:(const base::Value*)value {
  return NSObjectFromValue(value);
}

@end
