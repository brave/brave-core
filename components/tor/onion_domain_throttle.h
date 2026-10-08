/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_TOR_ONION_DOMAIN_THROTTLE_H_
#define BRAVE_COMPONENTS_TOR_ONION_DOMAIN_THROTTLE_H_

#include <memory>

#include "base/memory/weak_ptr.h"
#include "services/network/public/mojom/url_response_head.mojom-forward.h"
#include "third_party/blink/public/common/loader/url_loader_throttle.h"

namespace net {
struct RedirectInfo;
}

namespace network {
struct HttpRequestHeadersUpdateParams;
}

namespace tor {

// For blocking non Tor windows subresources requests that contain onion
// domain
class OnionDomainThrottle : public blink::URLLoaderThrottle {
 public:
  ~OnionDomainThrottle() override;
  OnionDomainThrottle(const OnionDomainThrottle&) = delete;
  OnionDomainThrottle& operator=(const OnionDomainThrottle&) = delete;

  static std::unique_ptr<blink::URLLoaderThrottle> MaybeCreateThrottle(
      bool is_onion_allowed);
  // For the browser-side keepalive loader, which deletes itself when a
  // throttle cancels it synchronously from WillStartRequest(). The returned
  // throttle defers the request and cancels it on the next task instead.
  static std::unique_ptr<blink::URLLoaderThrottle>
  MaybeCreateThrottleForKeepAlive(bool is_onion_allowed);

  // blink::URLLoaderThrottle
  void WillStartRequest(network::ResourceRequest* request,
                        bool* defer) override;
  void WillRedirectRequest(
      net::RedirectInfo* redirect_info,
      const network::mojom::URLResponseHead& response_head,
      bool* defer,
      network::HttpRequestHeadersUpdateParams* headers_update_params) override;
  void DetachFromCurrentSequence() override {}

 private:
  explicit OnionDomainThrottle(bool cancel_start_async);

  void CancelRequest();

  const bool cancel_start_async_;
  base::WeakPtrFactory<OnionDomainThrottle> weak_ptr_factory_{this};
};

}  // namespace tor

#endif  // BRAVE_COMPONENTS_TOR_ONION_DOMAIN_THROTTLE_H_
