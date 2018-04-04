/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_network_delegate_base.h"

#include <algorithm>

#include "brave/browser/net/url_context.h"
#include "content/public/browser/browser_thread.h"
#include "net/url_request/url_request.h"


using content::BrowserThread;

BraveNetworkDelegateBase::BraveNetworkDelegateBase(
    extensions::EventRouterForwarder* event_router,
    BooleanPrefMember* enable_referrers) :
    ChromeNetworkDelegate(event_router, enable_referrers) {
}

BraveNetworkDelegateBase::~BraveNetworkDelegateBase() {
}

int BraveNetworkDelegateBase::OnBeforeURLRequest(net::URLRequest* request,
    const net::CompletionCallback& callback,
    GURL* new_url) {
  if (before_url_request_callbacks_.empty() || !request) {
    return ChromeNetworkDelegate::OnBeforeURLRequest(request, callback, new_url);
  }
  std::shared_ptr<brave::BraveRequestInfo> ctx(
      new brave::BraveRequestInfo());
  callbacks_[request->identifier()] = callback;
  ctx->request_identifier = request->identifier();
  ctx->event_type = brave::kOnBeforeRequest;
  RunNextCallback(request, new_url, ctx);
  return net::ERR_IO_PENDING;
}

int BraveNetworkDelegateBase::OnBeforeStartTransaction(net::URLRequest* request,
    const net::CompletionCallback& callback,
    net::HttpRequestHeaders* headers) {
  if (before_start_transaction_callbacks_.empty() || !request) {
    return ChromeNetworkDelegate::OnBeforeStartTransaction(request, callback,
                                                           headers);
  }
  std::shared_ptr<brave::BraveRequestInfo> ctx(
      new brave::BraveRequestInfo());
  callbacks_[request->identifier()] = callback;
  ctx->headers = headers;
  ctx->request_identifier = request->identifier();
  ctx->event_type = brave::kOnBeforeStartTransaction;
  RunNextCallback(request, nullptr, ctx);
  return net::ERR_IO_PENDING;
}

void BraveNetworkDelegateBase::RunNextCallback(
    net::URLRequest* request,
    GURL* new_url,
    std::shared_ptr<brave::BraveRequestInfo> ctx) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  if (!ContainsKey(callbacks_, ctx->request_identifier)) {
    return;
  }

  // Continue processing callbacks until we hit one that returns PENDING
  int rv = net::OK;

  if (ctx->event_type == brave::kOnBeforeRequest) {
    while(before_url_request_callbacks_.size() != ctx->next_url_request_index) {
      brave::OnBeforeURLRequestCallback callback =
          before_url_request_callbacks_[ctx->next_url_request_index++];
      brave::ResponseCallback next_callback =
          base::Bind(&BraveNetworkDelegateBase::RunNextCallback,
              base::Unretained(this), request, new_url, ctx);
      rv = callback.Run(request, new_url, next_callback, ctx);
      if (rv == net::ERR_IO_PENDING) {
        return;
      }
      if (rv == net::ERR_ABORTED) {
        break;
      }
    }
  } else if (ctx->event_type == brave::kOnBeforeStartTransaction) {
    while(before_start_transaction_callbacks_.size() != ctx->next_url_request_index) {
      brave::OnBeforeStartTransactionCallback callback =
          before_start_transaction_callbacks_[ctx->next_url_request_index++];
      brave::ResponseCallback next_callback =
          base::Bind(&BraveNetworkDelegateBase::RunNextCallback,
              base::Unretained(this), request, new_url, ctx);
      rv = callback.Run(request, ctx->headers, next_callback, ctx);
      if (rv == net::ERR_IO_PENDING) {
        return;
      }
      if (rv == net::ERR_ABORTED) {
        break;
      }
    }
  }

  const net::CompletionCallback& net_completion_callback =
    callbacks_[ctx->request_identifier];
  if (rv == net::ERR_ABORTED) {
    net_completion_callback.Run(rv);
    return;
  }

  if (ctx->event_type == brave::kOnBeforeRequest) {
    rv = ChromeNetworkDelegate::OnBeforeURLRequest(request,
        net_completion_callback, new_url);
  } else if (ctx->event_type == brave::kOnBeforeStartTransaction) {
    rv = ChromeNetworkDelegate::OnBeforeStartTransaction(request,
        net_completion_callback, ctx->headers);
  }

  if (rv != net::ERR_IO_PENDING) {
    net_completion_callback.Run(rv);
  }
}

void BraveNetworkDelegateBase::OnURLRequestDestroyed(net::URLRequest* request) {
  if (ContainsKey(callbacks_, request->identifier())) {
    callbacks_.erase(request->identifier());
  }
  ChromeNetworkDelegate::OnURLRequestDestroyed(request);
}
