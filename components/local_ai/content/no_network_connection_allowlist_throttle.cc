/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/local_ai/content/no_network_connection_allowlist_throttle.h"

#include "base/check.h"
#include "brave/components/local_ai/core/url_constants.h"
#include "content/public/common/url_constants.h"
#include "net/http/http_response_headers.h"
#include "services/network/public/cpp/connection_allowlist_parser.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "url/gurl.h"

namespace local_ai {

// static
std::unique_ptr<blink::URLLoaderThrottle>
NoNetworkConnectionAllowlistThrottle::MaybeCreateThrottleFor(const GURL& url) {
  if (!url.SchemeIs(content::kChromeUIUntrustedScheme) ||
      url.host() != kOnDeviceSpeechRecognitionWorkerHost) {
    return nullptr;
  }
  return std::make_unique<NoNetworkConnectionAllowlistThrottle>();
}

NoNetworkConnectionAllowlistThrottle::NoNetworkConnectionAllowlistThrottle() =
    default;
NoNetworkConnectionAllowlistThrottle::~NoNetworkConnectionAllowlistThrottle() =
    default;

void NoNetworkConnectionAllowlistThrottle::WillProcessResponse(
    const GURL& response_url,
    network::mojom::URLResponseHead* response_head,
    bool* defer) {
  // The WebUI loader sets and parses the headers of every response. Chromium
  // reads the parsed copy, and debug builds check that it matches the headers.
  CHECK(response_head->headers);
  CHECK(response_head->parsed_headers);
  response_head->headers->SetHeader("Connection-Allowlist", "()");
  response_head->parsed_headers->connection_allowlists =
      network::ParseConnectionAllowlistsFromHeaders(*response_head->headers,
                                                    response_url);
}

}  // namespace local_ai
