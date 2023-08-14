/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/tor/renderer/onion_domain_throttle.h"

#include "net/base/net_errors.h"
#include "net/base/url_util.h"
#include "services/network/public/cpp/resource_request.h"

namespace tor {

OnionDomainThrottle::OnionDomainThrottle(
    base::RepeatingCallback<bool()> read_is_tor_cb)
    : read_is_tor_cb_(read_is_tor_cb) {}

OnionDomainThrottle::~OnionDomainThrottle() = default;

void OnionDomainThrottle::WillStartRequest(network::ResourceRequest* request,
                                           bool* defer) {
  bool is_tor = read_is_tor_cb_.Run();
  if (!is_tor && net::IsOnion(request->url)) {
    delegate_->CancelWithError(net::ERR_BLOCKED_BY_CLIENT);
  }
}

}  // namespace tor
