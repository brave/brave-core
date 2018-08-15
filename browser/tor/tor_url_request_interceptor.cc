/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/tor/tor_url_request_interceptor.h"
#include "net/url_request/url_request_job.h"

#include "base/logging.h"

namespace tor {

TorURLRequestInterceptor::TorURLRequestInterceptor(
    content::BrowserContext* browser_context) :
      browser_context_(browser_context) { LOG(ERROR) << browser_context_;}
TorURLRequestInterceptor::~TorURLRequestInterceptor() {}

net::URLRequestJob* TorURLRequestInterceptor::MaybeInterceptRequest(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate) const {
  // TODO: set proxy config for request
  return nullptr;
}

net::URLRequestJob* TorURLRequestInterceptor::MaybeInterceptRedirect(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate,
    const GURL& location) const {
  // TODO: set proxy config for request
  return nullptr;
}

}  // namespace tor
