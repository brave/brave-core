/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_network_delegate.h"

#include <algorithm>

#include "brave/browser/net/brave_httpse_network_delegate.h"
#include "brave/browser/net/url_context.h"

//#include "chrome/browser/profiles/profile_io_data.h"
#include "content/public/browser/browser_thread.h"
//#include "content/public/browser/resource_request_info.h"
#include "net/url_request/url_request.h"


using content::BrowserThread;

BraveNetworkDelegate::ResponseListenerInfo::ResponseListenerInfo() {
}

BraveNetworkDelegate::ResponseListenerInfo::~ResponseListenerInfo() {
}

BraveNetworkDelegate::BraveNetworkDelegate(
    extensions::EventRouterForwarder* event_router,
    BooleanPrefMember* enable_referrers) :
    ChromeNetworkDelegate(event_router, enable_referrers) {
  brave::OnBeforeURLRequestCallback callback =
      base::Bind(
          brave::OnBeforeURLRequest_HttpsePreFileWork);
  before_url_request_callbacks_.push_back(callback);
}

BraveNetworkDelegate::~BraveNetworkDelegate() {
}

int BraveNetworkDelegate::OnBeforeURLRequest(net::URLRequest* request,
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

void BraveNetworkDelegate::RunNextCallback(
    net::URLRequest* request,
    GURL *new_url,
    std::shared_ptr<brave::OnBeforeURLRequestContext> ctx) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  if (!ContainsKey(callbacks_, ctx->request_identifier)) {
    return;
  }

  // Continue processing callbacks until we hit one that returns PENDING
  while(before_url_request_callbacks_.size() != ctx->next_url_request_index) {
    brave::OnBeforeURLRequestCallback callback =
        before_url_request_callbacks_[ctx->next_url_request_index++];
    brave::ResponseCallback next_callback =
        base::Bind(&BraveNetworkDelegate::RunNextCallback,
            base::Unretained(this), request, new_url, ctx);
    int rv = callback.Run(request, new_url, next_callback, ctx);
    if (rv == net::ERR_IO_PENDING) {
      return;
    }
  }

  const net::CompletionCallback& net_completion_callback =
    callbacks_[ctx->request_identifier];
  int rv =
    ChromeNetworkDelegate::OnBeforeURLRequest(request,
        net_completion_callback, new_url);
  if (rv != net::ERR_IO_PENDING) {
    net_completion_callback.Run(rv);
  }
}

void BraveNetworkDelegate::OnURLRequestDestroyed(net::URLRequest* request) {
  if (!ContainsKey(callbacks_, request->identifier())) {
    return;
  }
  callbacks_.erase(request->identifier());
}
