/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_NET_BRAVE_SITE_HACKS_NETWORK_DELEGATE_HELPER_H_
#define BRAVE_BROWSER_NET_BRAVE_SITE_HACKS_NETWORK_DELEGATE_HELPER_H_

#include "brave/browser/net/url_context.h"

namespace net {
class URLRequest;
}

namespace brave {

template <template <typename> class T>
int OnBeforeURLRequest_SiteHacksWork(const ResponseCallback& next_callback,
                                     T<BraveRequestInfo> ctx);

template <template <typename> class T>
int OnBeforeStartTransaction_SiteHacksWork(
    net::HttpRequestHeaders* headers,
    const ResponseCallback& next_callback,
    T<BraveRequestInfo> ctx);

}  // namespace brave

#endif  // BRAVE_BROWSER_NET_BRAVE_SITE_HACKS_NETWORK_DELEGATE_HELPER_H_
