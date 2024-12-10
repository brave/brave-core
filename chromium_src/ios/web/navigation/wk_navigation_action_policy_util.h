// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_IOS_WEB_NAVIGATION_WK_NAVIGATION_ACTION_POLICY_UTIL_H_
#define BRAVE_CHROMIUM_SRC_IOS_WEB_NAVIGATION_WK_NAVIGATION_ACTION_POLICY_UTIL_H_

#import <Foundation/Foundation.h>
#import <WebKit/WebKit.h>

#include "src/ios/web/navigation/wk_navigation_action_policy_util.h"  // IWYU pragma: export

namespace web {
class WebState;
}

namespace brave {
void ShouldBlockUniversalLinks(web::WebState* webState,
                               NSURLRequest* request,
                               bool* forceBlockUniversalLinks);
void ShouldBlockJavaScript(web::WebState* webState,
                           NSURLRequest* request,
                           WKWebpagePreferences* preferences);
}  // namespace brave

#endif  // BRAVE_CHROMIUM_SRC_IOS_WEB_NAVIGATION_WK_NAVIGATION_ACTION_POLICY_UTIL_H_
