/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_NET_FAKE_BRAVE_REQUEST_HANDLER_H_
#define BRAVE_BROWSER_NET_FAKE_BRAVE_REQUEST_HANDLER_H_

#include <optional>
#include <utility>
#include <vector>

#include "base/memory/scoped_refptr.h"
#include "brave/browser/net/brave_request_handler.h"
#include "brave/browser/net/url_context.h"
#include "net/base/completion_once_callback.h"
#include "net/base/net_errors.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace net {
class HttpRequestHeaders;
class HttpResponseHeaders;
}  // namespace net

// A minimal BraveRequestHandler used to unit test consumers such as
// BraveProxyingURLLoaderFactory and BraveProxyingWebSocket in isolation,
// without depending on BraveRequestHandlerImpl or any adblock /
// network-delegate machinery.
//
// The fake performs no request processing by default. It can optionally perform
// a single synthetic cross-origin redirect (mimicking a Brave static/adblock
// redirect) and records the request URL and the initiator that BraveRequestInfo
// carries for each request it sees.
template <template <typename> class T>
class FakeBraveRequestHandler : public BraveRequestHandler<T> {
 public:
  FakeBraveRequestHandler() = default;
  ~FakeBraveRequestHandler() override = default;

  // When a request for |from| is seen, rewrite it to |to|.
  void SetRedirect(const GURL& from, const GURL& to) {
    redirect_from_ = from;
    redirect_to_ = to;
  }

  using Observation = std::pair<GURL, std::optional<url::Origin>>;
  const std::vector<Observation>& observations() const { return observations_; }

  int OnBeforeURLRequest(T<brave::BraveRequestInfo> ctx,
                         net::CompletionOnceCallback callback,
                         GURL* new_url) override {
    observations_.emplace_back(ctx->request_url(), ctx->request_initiator());
    if (new_url && redirect_to_.is_valid() &&
        ctx->request_url() == redirect_from_) {
      *new_url = redirect_to_;
    }
    return net::OK;
  }

  int OnBeforeStartTransaction(T<brave::BraveRequestInfo> ctx,
                               net::CompletionOnceCallback callback,
                               net::HttpRequestHeaders* headers) override {
    return net::OK;
  }

  int OnHeadersReceived(
      T<brave::BraveRequestInfo> ctx,
      net::CompletionOnceCallback callback,
      const net::HttpResponseHeaders* original_response_headers,
      scoped_refptr<net::HttpResponseHeaders>* override_response_headers,
      GURL* allowed_unsafe_redirect_url) override {
    return net::OK;
  }

  void OnURLRequestDestroyed(T<brave::BraveRequestInfo> ctx) override {}

 private:
  GURL redirect_from_;
  GURL redirect_to_;
  std::vector<Observation> observations_;
};

#endif  // BRAVE_BROWSER_NET_FAKE_BRAVE_REQUEST_HANDLER_H_
