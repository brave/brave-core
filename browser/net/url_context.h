/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_NET_URL_CONTEXT_
#define BRAVE_BROWSER_NET_URL_CONTEXT_


#include <string>

#include "chrome/browser/net/chrome_network_delegate.h"
#include "url/gurl.h"

namespace brave {

enum BraveNetworkDelegateEventType {
  kOnBeforeRequest,
  kOnBeforeStartTransaction,
  kOnHeadersReceived,
  kUnknownEventType
};

struct BraveRequestInfo {
  BraveRequestInfo();
  ~BraveRequestInfo();
  GURL request_url;
  std::string new_url_spec;
  uint64_t request_identifier = 0;
  size_t next_url_request_index = 0;
  net::HttpRequestHeaders* headers = nullptr;
  const net::HttpResponseHeaders* original_response_headers = nullptr;
  scoped_refptr<net::HttpResponseHeaders>* override_response_headers = nullptr;
  GURL* allowed_unsafe_redirect_url = nullptr;
  BraveNetworkDelegateEventType event_type = kUnknownEventType;
  DISALLOW_COPY_AND_ASSIGN(BraveRequestInfo);
};

using ResponseCallback = base::Callback<void()>;

//ResponseListener
using OnBeforeURLRequestCallback =
    base::Callback<int(net::URLRequest* request,
        GURL* new_url,
        const ResponseCallback& next_callback,
        std::shared_ptr<BraveRequestInfo> ctx)>;
using OnBeforeStartTransactionCallback =
    base::Callback<int(net::URLRequest* request,
        net::HttpRequestHeaders* headers,
        const ResponseCallback& next_callback,
        std::shared_ptr<BraveRequestInfo> ctx)>;
using OnHeadersReceivedCallback =
    base::Callback<int(net::URLRequest* request,
        const net::HttpResponseHeaders* original_response_headers,
        scoped_refptr<net::HttpResponseHeaders>* override_response_headers,
        GURL* allowed_unsafe_redirect_url,
        const ResponseCallback& next_callback,
        std::shared_ptr<BraveRequestInfo> ctx)>;

}  // namespace brave


#endif  // BRAVE_BROWSER_NET_URL_CONTEXT_
