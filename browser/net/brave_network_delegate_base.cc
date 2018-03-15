/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_network_delegate_base.h"

#include <algorithm>

#include "brave/browser/net/url_context.h"
#include "content/public/browser/browser_thread.h"
#include "net/url_request/url_request.h"


using content::BrowserThread;

BraveNetworkDelegateBase::ResponseListenerInfo::ResponseListenerInfo() {
}

BraveNetworkDelegateBase::ResponseListenerInfo::~ResponseListenerInfo() {
}

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

  std::shared_ptr<brave::OnBeforeURLRequestContext> ctx(
      new brave::OnBeforeURLRequestContext());

  callbacks_[request->identifier()] = callback;
  ctx->request_identifier = request->identifier();
  RunNextCallback(request, new_url, ctx);
  return net::ERR_IO_PENDING;
}

void BraveNetworkDelegateBase::RunNextCallback(
    net::URLRequest* request,
    GURL *new_url,
    std::shared_ptr<brave::OnBeforeURLRequestContext> ctx) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  if (!ContainsKey(callbacks_, ctx->request_identifier)) {
    return;
  }

  // Continue processing callbacks until we hit one that returns PENDING
  int rv = net::OK;
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

  const net::CompletionCallback& net_completion_callback =
    callbacks_[ctx->request_identifier];
  if (rv == net::ERR_ABORTED) {
    net_completion_callback.Run(rv);
    return;
  }

  rv =
    ChromeNetworkDelegate::OnBeforeURLRequest(request,
        net_completion_callback, new_url);
  if (rv != net::ERR_IO_PENDING) {
    net_completion_callback.Run(rv);
  }
}

void BraveNetworkDelegateBase::OnURLRequestDestroyed(net::URLRequest* request) {
  if (!ContainsKey(callbacks_, request->identifier())) {
    return;
  }
  callbacks_.erase(request->identifier());
}
