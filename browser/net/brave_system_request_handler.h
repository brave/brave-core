/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_NET_BRAVE_SYSTEM_REQUEST_HANDLER_H_
#define BRAVE_BROWSER_NET_BRAVE_SYSTEM_REQUEST_HANDLER_H_

#include <string>

namespace network {
struct ResourceRequest;
}

extern const char kBraveServicesKeyHeader[];

namespace brave {

std::string BraveServicesKeyForTesting();

void AddBraveServicesKeyHeader(network::ResourceRequest* url_request);

network::ResourceRequest OnBeforeSystemRequest(
    const network::ResourceRequest& url_request);

}  // namespace brave

#endif  // BRAVE_BROWSER_NET_BRAVE_SYSTEM_REQUEST_HANDLER_H_
