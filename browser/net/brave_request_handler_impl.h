/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_NET_BRAVE_REQUEST_HANDLER_IMPL_H_
#define BRAVE_BROWSER_NET_BRAVE_REQUEST_HANDLER_IMPL_H_

#include <cstdint>
#include <map>
#include <vector>

#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/browser/net/brave_request_handler.h"
#include "brave/browser/net/url_context.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/completion_once_callback.h"

class GURL;

namespace net {
class HttpRequestHeaders;
class HttpResponseHeaders;
}  // namespace net

// Production implementation of BraveRequestHandler. Wires up the concrete
// network-stack hooks (adblock, site hacks, static redirects, etc.).
template <template <typename> class T>
class BraveRequestHandlerImpl : public BraveRequestHandler<T> {
 public:
  BraveRequestHandlerImpl();
  BraveRequestHandlerImpl(const BraveRequestHandlerImpl&) = delete;
  BraveRequestHandlerImpl& operator=(const BraveRequestHandlerImpl&) = delete;
  ~BraveRequestHandlerImpl() override;

  bool IsRequestIdentifierValid(uint64_t request_identifier);

  int OnBeforeURLRequest(T<brave::BraveRequestInfo> ctx,
                         net::CompletionOnceCallback callback,
                         GURL* new_url) override;

  int OnBeforeStartTransaction(T<brave::BraveRequestInfo> ctx,
                               net::CompletionOnceCallback callback,
                               net::HttpRequestHeaders* headers) override;
  int OnHeadersReceived(
      T<brave::BraveRequestInfo> ctx,
      net::CompletionOnceCallback callback,
      const net::HttpResponseHeaders* original_response_headers,
      scoped_refptr<net::HttpResponseHeaders>* override_response_headers,
      GURL* allowed_unsafe_redirect_url) override;

  void OnURLRequestDestroyed(T<brave::BraveRequestInfo> ctx) override;
  void RunCallbackForRequestIdentifier(uint64_t request_identifier, int rv);

 private:
  void SetupCallbacks();
  void RunNextCallback(T<brave::BraveRequestInfo> ctx);

  std::vector<brave::OnBeforeURLRequestCallback<T>>
      before_url_request_callbacks_;
  std::vector<brave::OnBeforeStartTransactionCallback<T>>
      before_start_transaction_callbacks_;
  std::vector<brave::OnHeadersReceivedCallback<T>> headers_received_callbacks_;

  std::map<uint64_t, net::CompletionOnceCallback> callbacks_;

  base::WeakPtrFactory<BraveRequestHandlerImpl> weak_factory_{this};
};

#endif  // BRAVE_BROWSER_NET_BRAVE_REQUEST_HANDLER_IMPL_H_
