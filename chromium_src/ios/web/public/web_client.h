// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_IOS_WEB_PUBLIC_WEB_CLIENT_H_
#define BRAVE_CHROMIUM_SRC_IOS_WEB_PUBLIC_WEB_CLIENT_H_

#import <Foundation/Foundation.h>

#include "ios/web/common/user_agent.h"

namespace web {
class WebState;
}

@class WKWebViewConfiguration;

// Add methods to override in BraveWebClient.
//
// `ShouldBlockJavaScript`, `GetUserAgentForRequest`, and
// `ShouldBlockUniversalLinks` will be called from BraveCRWWKNavigationHandler
// to allow us to implement a Brave specific features during web navigation
//
// `DidResetConfiguration` will be called from
// `BraveWKWebViewConfigurationProvider` to allow us to do some post-reset setup
// for Brave-specific configuration updates. This additional method will be
// removed in the future
#define IsBrowserLockdownModeEnabled                                           \
  ShouldBlockJavaScript(web::WebState* web_state, NSURLRequest* request);      \
  virtual NSString* GetUserAgentForRequest(web::WebState* web_state,           \
                                           web::UserAgentType user_agent_type, \
                                           NSURLRequest* request);             \
  virtual bool ShouldBlockUniversalLinks(web::WebState* web_state,             \
                                         NSURLRequest* request);               \
  virtual void DidResetConfiguration(web::BrowserState* browser_state,         \
                                     WKWebViewConfiguration* configuration);   \
  virtual bool IsBrowserLockdownModeEnabled
#include <ios/web/public/web_client.h>  // IWYU pragma: export
#undef IsBrowserLockdownModeEnabled

#endif  // BRAVE_CHROMIUM_SRC_IOS_WEB_PUBLIC_WEB_CLIENT_H_
