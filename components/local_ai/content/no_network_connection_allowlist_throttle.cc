/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/local_ai/content/no_network_connection_allowlist_throttle.h"

#include "services/network/public/cpp/connection_allowlist.h"
#include "services/network/public/mojom/parsed_headers.mojom.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "url/gurl.h"

namespace local_ai {

NoNetworkConnectionAllowlistThrottle::NoNetworkConnectionAllowlistThrottle() =
    default;
NoNetworkConnectionAllowlistThrottle::~NoNetworkConnectionAllowlistThrottle() =
    default;

void NoNetworkConnectionAllowlistThrottle::WillProcessResponse(
    const GURL& response_url,
    network::mojom::URLResponseHead* response_head,
    bool* defer) {
  if (!response_head || !response_head->headers) {
    return;
  }
  if (!response_head->parsed_headers) {
    response_head->parsed_headers = network::mojom::ParsedHeaders::New();
  }
  // An enforced allowlist with no patterns matches nothing => block all.
  response_head->parsed_headers->connection_allowlists.enforced =
      network::ConnectionAllowlist();
}

}  // namespace local_ai
