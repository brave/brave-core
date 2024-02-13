/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/tor/renderer/onion_domain_throttle.h"

#include "base/memory/ptr_util.h"
#include "net/base/net_errors.h"
#include "net/base/url_util.h"
#include "services/network/public/cpp/resource_request.h"

namespace tor {

OnionDomainThrottle::OnionDomainThrottle() = default;

OnionDomainThrottle::~OnionDomainThrottle() = default;

// static
std::unique_ptr<blink::URLLoaderThrottle>
OnionDomainThrottle::MaybeCreateThrottle(bool is_onion_allowed) {
  if (is_onion_allowed) {
    return nullptr;
  }
  return base::WrapUnique(new tor::OnionDomainThrottle());
}

void OnionDomainThrottle::WillStartRequest(network::ResourceRequest* request,
                                           bool* defer) {
  if (net::IsOnion(request->url)) {
    delegate_->CancelWithError(net::ERR_BLOCKED_BY_CLIENT);
  }
}

}  // namespace tor
