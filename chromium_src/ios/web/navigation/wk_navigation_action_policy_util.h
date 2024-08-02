// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_IOS_WEB_NAVIGATION_WK_NAVIGATION_ACTION_POLICY_UTIL_H_
#define BRAVE_CHROMIUM_SRC_IOS_WEB_NAVIGATION_WK_NAVIGATION_ACTION_POLICY_UTIL_H_

#import <Foundation/Foundation.h>

#include "src/ios/web/navigation/wk_navigation_action_policy_util.h"

namespace web {
class WebState;
}

namespace brave {
bool ShouldBlockUniversalLinks(web::WebState* webState,
                               NSURLRequest* request,
                               bool* forceBlockUniversalLinks);
}

#endif  // BRAVE_CHROMIUM_SRC_IOS_WEB_NAVIGATION_WK_NAVIGATION_ACTION_POLICY_UTIL_H_
