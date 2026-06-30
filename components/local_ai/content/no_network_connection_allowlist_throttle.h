/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_LOCAL_AI_CONTENT_NO_NETWORK_CONNECTION_ALLOWLIST_THROTTLE_H_
#define BRAVE_COMPONENTS_LOCAL_AI_CONTENT_NO_NETWORK_CONNECTION_ALLOWLIST_THROTTLE_H_

#include "third_party/blink/public/common/loader/url_loader_throttle.h"

namespace local_ai {

// Injects an enforced, empty Connection-Allowlist into the on-device model
// worker WebUIs' navigation response. Chromium then enforces it via the network
// service for the document's network_restrictions_id, blocking every
// fetch/XHR/WebSocket connection it makes (and WebTransport/DNS once the
// Chromium pin includes those checks), browser-side and independent of CSP.
// fetch is already denied by the non-network WebUI factory; this closes the
// socket connectors. Going through the response-header path (rather than
// calling NetworkContext::RestrictNetworkForIds directly) lets Chromium
// re-apply the restriction after a NetworkService crash and tear it down with
// the document.
class NoNetworkConnectionAllowlistThrottle : public blink::URLLoaderThrottle {
 public:
  NoNetworkConnectionAllowlistThrottle();
  ~NoNetworkConnectionAllowlistThrottle() override;

  // blink::URLLoaderThrottle:
  void WillProcessResponse(const GURL& response_url,
                           network::mojom::URLResponseHead* response_head,
                           bool* defer) override;
};

}  // namespace local_ai

#endif  // BRAVE_COMPONENTS_LOCAL_AI_CONTENT_NO_NETWORK_CONNECTION_ALLOWLIST_THROTTLE_H_
