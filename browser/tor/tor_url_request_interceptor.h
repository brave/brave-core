/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_TOR_TOR_URL_REQUEST_INTERCEPTOR_
#define BRAVE_BROWSER_TOR_TOR_URL_REQUEST_INTERCEPTOR_

#include "base/macros.h"
#include "net/url_request/url_request_interceptor.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace tor {

class TorURLRequestInterceptor : public net::URLRequestInterceptor {
 public:

  explicit TorURLRequestInterceptor(content::BrowserContext* browser_context);
  ~TorURLRequestInterceptor() override;

  // net::URLRequestInterceptor implementation.
  net::URLRequestJob* MaybeInterceptRequest(
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate) const override;

  net::URLRequestJob* MaybeInterceptRedirect(
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate,
      const GURL& location) const override;

 private:
  content::BrowserContext* browser_context_; // NOT OWNED
  DISALLOW_COPY_AND_ASSIGN(TorURLRequestInterceptor);
};

}  // namespace tor

#endif  // BRAVE_BROWSER_TOR_TOR_URL_REQUEST_INTERCEPTOR_
