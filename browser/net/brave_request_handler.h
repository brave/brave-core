/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_NET_BRAVE_REQUEST_HANDLER_H_
#define BRAVE_BROWSER_NET_BRAVE_REQUEST_HANDLER_H_

#include "base/memory/scoped_refptr.h"
#include "brave/browser/net/url_context.h"
#include "net/base/completion_once_callback.h"

class GURL;

namespace net {
class HttpRequestHeaders;
class HttpResponseHeaders;
}  // namespace net

// Interface for the different network stack hooks (similar to capabilities of
// the WebRequest API). BraveRequestHandlerImpl is the production impl.
// This interface exists so consumers such as BraveProxyingURLLoaderFactory can
// be unit tested with a fake handler, without pulling in adblock or other
// network-delegate dependencies.
template <template <typename> class T>
class BraveRequestHandler {
 public:
  virtual ~BraveRequestHandler() = default;

  virtual int OnBeforeURLRequest(T<brave::BraveRequestInfo> ctx,
                                 net::CompletionOnceCallback callback,
                                 GURL* new_url) = 0;

  virtual int OnBeforeStartTransaction(T<brave::BraveRequestInfo> ctx,
                                       net::CompletionOnceCallback callback,
                                       net::HttpRequestHeaders* headers) = 0;

  virtual int OnHeadersReceived(
      T<brave::BraveRequestInfo> ctx,
      net::CompletionOnceCallback callback,
      const net::HttpResponseHeaders* original_response_headers,
      scoped_refptr<net::HttpResponseHeaders>* override_response_headers,
      GURL* allowed_unsafe_redirect_url) = 0;

  virtual void OnURLRequestDestroyed(T<brave::BraveRequestInfo> ctx) = 0;

 protected:
  BraveRequestHandler() = default;
};

#endif  // BRAVE_BROWSER_NET_BRAVE_REQUEST_HANDLER_H_
