/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_NET_URL_CONTEXT_
#define BRAVE_BROWSER_NET_URL_CONTEXT_


#include <string>

#include "chrome/browser/net/chrome_network_delegate.h"
#include "url/gurl.h"

namespace brave {

struct OnBeforeURLRequestContext {
  OnBeforeURLRequestContext();
  ~OnBeforeURLRequestContext();
  GURL request_url;
  std::string new_url_spec;
  uint64_t request_identifier = 0;
  size_t next_url_request_index = 0;
  DISALLOW_COPY_AND_ASSIGN(OnBeforeURLRequestContext);
};

using ResponseCallback = base::Callback<void()>;

//ResponseListener
using OnBeforeURLRequestCallback =
    base::Callback<int(net::URLRequest* request,
        GURL* new_url,
        const ResponseCallback& next_callback,
        std::shared_ptr<OnBeforeURLRequestContext> ctx)>;

}  // namespace brave


#endif  // BRAVE_BROWSER_NET_URL_CONTEXT_
