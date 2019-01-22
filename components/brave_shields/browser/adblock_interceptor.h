/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CONTENT_BROWSER_ADBLOCK_INTERCEPTOR_H_
#define BRAVE_CONTENT_BROWSER_ADBLOCK_INTERCEPTOR_H_

#include "base/macros.h"
#include "content/common/content_export.h"
#include "net/url_request/url_request_interceptor.h"

namespace brave_shields {

// Intercepts certain requests and blocks them by silently returning 200 OK
// and not allowing them to hit the network.
class CONTENT_EXPORT AdBlockInterceptor : public net::URLRequestInterceptor {
 public:
  AdBlockInterceptor();
  ~AdBlockInterceptor() override;

 protected:
  // net::URLRequestInterceptor:
  net::URLRequestJob* MaybeInterceptRequest(
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate) const override;
};

}  // namespace brave_shields

#endif  // BRAVE_CONTENT_BROWSER_ADBLOCK_INTERCEPTOR_H_
