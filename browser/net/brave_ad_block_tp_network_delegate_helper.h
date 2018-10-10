/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_NET_BRAVE_AD_BLOCK_TP_NETWORK_DELEGATE_H_
#define BRAVE_BROWSER_NET_BRAVE_AD_BLOCK_TP_NETWORK_DELEGATE_H_

#include "brave/browser/net/url_context.h"

struct BraveRequestInfo;

namespace net {
class URLRequest;
}

namespace brave {

int OnBeforeURLRequest_AdBlockTPWork(
    net::URLRequest* request,
    GURL* new_url,
    const ResponseCallback& next_callback,
    std::shared_ptr<BraveRequestInfo> ctx);

bool GetPolyfillForAdBlock(bool allow_brave_shields, bool allow_ads,
    const GURL& tab_origin, const GURL& gurl, GURL *new_url);

}  // namespace brave

#endif  // BRAVE_BROWSER_NET_BRAVE_AD_BLOCK_TP_NETWORK_DELEGATE_H_
