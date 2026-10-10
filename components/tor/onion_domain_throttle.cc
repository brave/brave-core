/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/tor/onion_domain_throttle.h"

#include "base/functional/bind.h"
#include "base/location.h"
#include "base/memory/ptr_util.h"
#include "base/task/sequenced_task_runner.h"
#include "net/base/net_errors.h"
#include "net/base/url_util.h"
#include "net/url_request/redirect_info.h"
#include "services/network/public/cpp/resource_request.h"

namespace tor {

OnionDomainThrottle::OnionDomainThrottle(bool cancel_start_async)
    : cancel_start_async_(cancel_start_async) {}

OnionDomainThrottle::~OnionDomainThrottle() = default;

// static
std::unique_ptr<blink::URLLoaderThrottle>
OnionDomainThrottle::MaybeCreateThrottle(bool is_onion_allowed) {
  if (is_onion_allowed) {
    return nullptr;
  }
  return base::WrapUnique(
      new tor::OnionDomainThrottle(/*cancel_start_async=*/false));
}

// static
std::unique_ptr<blink::URLLoaderThrottle>
OnionDomainThrottle::MaybeCreateThrottleForKeepAlive(bool is_onion_allowed) {
  if (is_onion_allowed) {
    return nullptr;
  }
  return base::WrapUnique(
      new tor::OnionDomainThrottle(/*cancel_start_async=*/true));
}

void OnionDomainThrottle::WillStartRequest(network::ResourceRequest* request,
                                           bool* defer) {
  if (!net::IsOnion(request->url)) {
    return;
  }
  if (cancel_start_async_) {
    *defer = true;
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, base::BindOnce(&OnionDomainThrottle::CancelRequest,
                                  weak_ptr_factory_.GetWeakPtr()));
    return;
  }
  CancelRequest();
}

void OnionDomainThrottle::WillRedirectRequest(
    net::RedirectInfo* redirect_info,
    const network::mojom::URLResponseHead& response_head,
    bool* defer,
    network::HttpRequestHeadersUpdateParams* headers_update_params) {
  if (net::IsOnion(redirect_info->new_url)) {
    CancelRequest();
  }
}

void OnionDomainThrottle::CancelRequest() {
  delegate_->CancelWithError(net::ERR_NAME_NOT_RESOLVED);
}

}  // namespace tor
