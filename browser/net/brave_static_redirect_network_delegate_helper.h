/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_NET_BRAVE_STATIC_REDIRECT_NETWORK_DELEGATE_HELPER_H_
#define BRAVE_BROWSER_NET_BRAVE_STATIC_REDIRECT_NETWORK_DELEGATE_HELPER_H_

#include <memory>
#include <string>

#include "brave/browser/net/url_context.h"

struct BraveRequestInfo;

namespace brave {

extern const char kSafeBrowsingTestingEndpoint[];

int OnBeforeURLRequest_StaticRedirectWork(
    const ResponseCallback& next_callback,
    std::shared_ptr<BraveRequestInfo> ctx);

int OnBeforeURLRequest_StaticRedirectWorkForGURL(
    const GURL& request_url,
    GURL* new_url);

void SetSafeBrowsingEndpointForTesting(bool testing);

}  // namespace brave

#endif  // BRAVE_BROWSER_NET_BRAVE_STATIC_REDIRECT_NETWORK_DELEGATE_HELPER_H_
