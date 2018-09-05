/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_NET_BRAVE_COMMON_STATIC_REDIRECT_NETWORK_DELEGATE_H_
#define BRAVE_BROWSER_NET_BRAVE_COMMON_STATIC_REDIRECT_NETWORK_DELEGATE_H_

#include "chrome/browser/net/chrome_network_delegate.h"
#include "brave/browser/net/url_context.h"

struct BraveRequestInfo;

namespace net {
class URLRequest;
}

namespace brave {

int OnBeforeURLRequest_CommonStaticRedirectWork(
    net::URLRequest* request,
    GURL* new_url,
    const ResponseCallback& next_callback,
    std::shared_ptr<BraveRequestInfo> ctx);

}  // namespace brave

#endif  // BRAVE_BROWSER_NET_BRAVE_COMMON_STATIC_REDIRECT_NETWORK_DELEGATE_H_
