/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WEBTORRENT_BROWSER_NET_BRAVE_TORRENT_REDIRECT_NETWORK_DELEGATE_HELPER_H_
#define BRAVE_COMPONENTS_BRAVE_WEBTORRENT_BROWSER_NET_BRAVE_TORRENT_REDIRECT_NETWORK_DELEGATE_HELPER_H_

#include "chrome/browser/net/chrome_network_delegate.h"
#include "brave/browser/net/url_context.h"

namespace brave {
struct BraveRequestInfo;
}

namespace net {
class URLRequest;
}

namespace webtorrent {

int OnHeadersReceived_TorrentRedirectWork(
    net::URLRequest* request,
    const net::HttpResponseHeaders* original_response_headers,
    scoped_refptr<net::HttpResponseHeaders>* override_response_headers,
    GURL* allowed_unsafe_redirect_url,
    const brave::ResponseCallback& next_callback,
    std::shared_ptr<brave::BraveRequestInfo> ctx);

}  // namespace webtorrent

#endif  // BRAVE_COMPONENTS_BRAVE_WEBTORRENT_BROWSER_NET_BRAVE_TORRENT_REDIRECT_NETWORK_DELEGATE_HELPER_H_
