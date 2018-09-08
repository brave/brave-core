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
    extensions::EventRouterForwarder* event_router) :
    ChromeNetworkDelegate(event_router) {
}

BraveNetworkDelegateBase::~BraveNetworkDelegateBase() {
}

int BraveNetworkDelegateBase::OnBeforeURLRequest(net::URLRequest* request,
    net::CompletionOnceCallback callback,
    GURL* new_url) {
  if (before_url_request_callbacks_.empty() || !request) {
    return ChromeNetworkDelegate::OnBeforeURLRequest(request, std::move(callback), new_url);
  }
  std::shared_ptr<brave::BraveRequestInfo> ctx(
      new brave::BraveRequestInfo());
  callbacks_[request->identifier()] = std::move(callback);
  ctx->request_identifier = request->identifier();
  ctx->event_type = brave::kOnBeforeRequest;
  RunNextCallback(request, new_url, ctx);
  return net::ERR_IO_PENDING;
}

int BraveNetworkDelegateBase::OnBeforeStartTransaction(net::URLRequest* request,
    net::CompletionOnceCallback callback,
    net::HttpRequestHeaders* headers) {
  if (before_start_transaction_callbacks_.empty() || !request) {
    return ChromeNetworkDelegate::OnBeforeStartTransaction(request, std::move(callback),
                                                           headers);
  }
  std::shared_ptr<brave::BraveRequestInfo> ctx(
      new brave::BraveRequestInfo());
  callbacks_[request->identifier()] = std::move(callback);
  ctx->headers = headers;
  ctx->request_identifier = request->identifier();
  ctx->event_type = brave::kOnBeforeStartTransaction;
  RunNextCallback(request, nullptr, ctx);
  return net::ERR_IO_PENDING;
}

int BraveNetworkDelegateBase::OnHeadersReceived(net::URLRequest* request,
      net::CompletionOnceCallback callback,
      const net::HttpResponseHeaders* original_response_headers,
      scoped_refptr<net::HttpResponseHeaders>* override_response_headers,
      GURL* allowed_unsafe_redirect_url) {
  if (headers_received_callbacks_.empty() || !request) {
    return ChromeNetworkDelegate::OnHeadersReceived(request,
        std::move(callback), original_response_headers,
        override_response_headers, allowed_unsafe_redirect_url);
  }

  std::shared_ptr<brave::BraveRequestInfo> ctx(
      new brave::BraveRequestInfo());
  callbacks_[request->identifier()] = std::move(callback);
  ctx->request_identifier = request->identifier();
  ctx->event_type = brave::kOnHeadersReceived;
  ctx->original_response_headers = original_response_headers;
  ctx->override_response_headers = override_response_headers;
  ctx->allowed_unsafe_redirect_url = allowed_unsafe_redirect_url;

  // Return ERR_IO_PENDING and run callbacks later by posting a task.
  // URLRequestHttpJob::awaiting_callback_ will be set to true after we
  // return net::ERR_IO_PENDING here, callbacks need to be run later then this
  // to set awaiting_callback_ back to false.
  BrowserThread::PostTask(BrowserThread::IO, FROM_HERE,
      base::Bind(&BraveNetworkDelegateBase::RunNextCallback,
        base::Unretained(this), request, nullptr, ctx));
  return net::ERR_IO_PENDING;
}

void BraveNetworkDelegateBase::RunCallbackForRequestIdentifier(uint64_t request_identifier, int rv) {
  std::map<uint64_t, net::CompletionOnceCallback>::iterator it =
      callbacks_.find(request_identifier);
  std::move(it->second).Run(rv);
}

void BraveNetworkDelegateBase::RunNextCallback(
    net::URLRequest* request,
    GURL* new_url,
    std::shared_ptr<brave::BraveRequestInfo> ctx) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  if (!ContainsKey(callbacks_, ctx->request_identifier)) {
    return;
  }

  if (request->status().status() == net::URLRequestStatus::CANCELED) {
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
  } else if (ctx->event_type == brave::kOnHeadersReceived) {
    while(headers_received_callbacks_.size() != ctx->next_url_request_index) {
      brave::OnHeadersReceivedCallback callback =
          headers_received_callbacks_[ctx->next_url_request_index++];
      brave::ResponseCallback next_callback =
          base::Bind(&BraveNetworkDelegateBase::RunNextCallback,
              base::Unretained(this), request, new_url, ctx);
      rv = callback.Run(request, ctx->original_response_headers,
          ctx->override_response_headers, ctx->allowed_unsafe_redirect_url,
          next_callback, ctx);
      if (rv == net::ERR_IO_PENDING) {
        return;
      }
      if (rv == net::ERR_ABORTED) {
        break;
      }
    }
  }

  std::map<uint64_t, net::CompletionOnceCallback>::iterator it =
      callbacks_.find(ctx->request_identifier);
  if (rv == net::ERR_ABORTED) {
    std::move(it->second).Run(rv);
    return;
  }

  net::CompletionOnceCallback wrapped_callback = base::BindOnce(
      &BraveNetworkDelegateBase::RunCallbackForRequestIdentifier, base::Unretained(this), ctx->request_identifier);

  if (ctx->event_type == brave::kOnBeforeRequest) {
    rv = ChromeNetworkDelegate::OnBeforeURLRequest(request,
        std::move(wrapped_callback), new_url);
  } else if (ctx->event_type == brave::kOnBeforeStartTransaction) {
    rv = ChromeNetworkDelegate::OnBeforeStartTransaction(request,
        std::move(wrapped_callback), ctx->headers);
  } else if (ctx->event_type == brave::kOnHeadersReceived) {
    rv = ChromeNetworkDelegate::OnHeadersReceived(request,
        std::move(wrapped_callback), ctx->original_response_headers,
        ctx->override_response_headers, ctx->allowed_unsafe_redirect_url);
  }

  // ChromeNetworkDelegate returns net::ERR_IO_PENDING if an extension is
  // intercepting the request and OK if the request should proceed normally.
  if (rv != net::ERR_IO_PENDING) {
    RunCallbackForRequestIdentifier(ctx->request_identifier, rv);
  }
}

void BraveNetworkDelegateBase::OnURLRequestDestroyed(net::URLRequest* request) {
  if (ContainsKey(callbacks_, request->identifier())) {
    callbacks_.erase(request->identifier());
  }
  ChromeNetworkDelegate::OnURLRequestDestroyed(request);
}
