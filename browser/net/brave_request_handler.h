/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_NET_BRAVE_REQUEST_HANDLER_H_
#define BRAVE_BROWSER_NET_BRAVE_REQUEST_HANDLER_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/memory/scoped_refptr.h"
#include "brave/browser/net/url_context.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/completion_once_callback.h"

class PrefChangeRegistrar;

// Contains different network stack hooks (similar to capabilities of WebRequest
// API).
template <template <typename> class T>
class BraveRequestHandler {
 public:
  BraveRequestHandler();
  BraveRequestHandler(const BraveRequestHandler&) = delete;
  BraveRequestHandler& operator=(const BraveRequestHandler&) = delete;
  ~BraveRequestHandler();

  bool IsRequestIdentifierValid(uint64_t request_identifier);

  int OnBeforeURLRequest(T<brave::BraveRequestInfo> ctx,
                         net::CompletionOnceCallback callback,
                         GURL* new_url);

  int OnBeforeStartTransaction(T<brave::BraveRequestInfo> ctx,
                               net::CompletionOnceCallback callback,
                               net::HttpRequestHeaders* headers);
  int OnHeadersReceived(
      T<brave::BraveRequestInfo> ctx,
      net::CompletionOnceCallback callback,
      const net::HttpResponseHeaders* original_response_headers,
      scoped_refptr<net::HttpResponseHeaders>* override_response_headers,
      GURL* allowed_unsafe_redirect_url);

  void OnURLRequestDestroyed(T<brave::BraveRequestInfo> ctx);
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

  base::WeakPtrFactory<BraveRequestHandler> weak_factory_{this};
};

#endif  // BRAVE_BROWSER_NET_BRAVE_REQUEST_HANDLER_H_
