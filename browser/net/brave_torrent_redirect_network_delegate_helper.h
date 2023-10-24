/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_NET_BRAVE_TORRENT_REDIRECT_NETWORK_DELEGATE_HELPER_H_
#define BRAVE_BROWSER_NET_BRAVE_TORRENT_REDIRECT_NETWORK_DELEGATE_HELPER_H_

#include <memory>

#include "brave/browser/net/url_context.h"

namespace brave {
struct BraveRequestInfo;
}

namespace net {
class URLRequest;
}

namespace webtorrent {

// Determines whether a request should be redirected to a torrent file. This
// will occur if the following conditions are met:
// 1. The request succeeded
// 2. The request is in the Main frame
// 3. WebTorrent is enabled
// 4. The request is for a torrent file / or the WebTorrent extension initiated
// the request.
bool ShouldRedirectRequest(
    const net::HttpResponseHeaders* original_response_headers,
    std::shared_ptr<brave::BraveRequestInfo> ctx);

int OnHeadersReceived_TorrentRedirectWork(
    const net::HttpResponseHeaders* original_response_headers,
    scoped_refptr<net::HttpResponseHeaders>* override_response_headers,
    GURL* allowed_unsafe_redirect_url,
    const brave::ResponseCallback& next_callback,
    std::shared_ptr<brave::BraveRequestInfo> ctx);

}  // namespace webtorrent

#endif  // BRAVE_BROWSER_NET_BRAVE_TORRENT_REDIRECT_NETWORK_DELEGATE_HELPER_H_
