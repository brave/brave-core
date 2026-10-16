/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_LOCAL_AI_CONTENT_NO_NETWORK_CONNECTION_ALLOWLIST_THROTTLE_H_
#define BRAVE_COMPONENTS_LOCAL_AI_CONTENT_NO_NETWORK_CONNECTION_ALLOWLIST_THROTTLE_H_

#include <memory>

#include "third_party/blink/public/common/loader/url_loader_throttle.h"

class GURL;

namespace local_ai {

// Adds an empty `Connection-Allowlist` to every response from the speech
// worker origin. The page and its workers run WASM and must not reach the
// network. See https://github.com/WICG/connection-allowlists.
class NoNetworkConnectionAllowlistThrottle : public blink::URLLoaderThrottle {
 public:
  // Returns a throttle if `url` is on the speech worker origin, or nullptr.
  static std::unique_ptr<blink::URLLoaderThrottle> MaybeCreateThrottleFor(
      const GURL& url);

  NoNetworkConnectionAllowlistThrottle();
  ~NoNetworkConnectionAllowlistThrottle() override;

  // blink::URLLoaderThrottle:
  void WillProcessResponse(const GURL& response_url,
                           network::mojom::URLResponseHead* response_head,
                           bool* defer) override;
};

}  // namespace local_ai

#endif  // BRAVE_COMPONENTS_LOCAL_AI_CONTENT_NO_NETWORK_CONNECTION_ALLOWLIST_THROTTLE_H_
