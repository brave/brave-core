/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_TOR_RENDERER_ONION_DOMAIN_THROTTLE_H_
#define BRAVE_COMPONENTS_TOR_RENDERER_ONION_DOMAIN_THROTTLE_H_

#include "base/functional/callback.h"
#include "third_party/blink/public/common/loader/url_loader_throttle.h"

namespace tor {

// For blocking non Tor windows subresources requests that contain onion
// domain
class OnionDomainThrottle : public blink::URLLoaderThrottle {
 public:
  explicit OnionDomainThrottle(base::RepeatingCallback<bool()> read_is_tor_cb);
  ~OnionDomainThrottle() override;
  OnionDomainThrottle(const OnionDomainThrottle&) = delete;
  OnionDomainThrottle& operator=(const OnionDomainThrottle&) = delete;

  // blink::URLLoaderThrottle
  void WillStartRequest(network::ResourceRequest* request,
                        bool* defer) override;

 private:
  base::RepeatingCallback<bool()> read_is_tor_cb_;
};

}  // namespace tor

#endif  // BRAVE_COMPONENTS_TOR_RENDERER_ONION_DOMAIN_THROTTLE_H_
