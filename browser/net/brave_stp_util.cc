/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_stp_util.h"

#include <string>

#include "base/no_destructor.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"

namespace brave {

base::flat_set<base::StringPiece>* TrackableSecurityHeaders() {
  static base::NoDestructor<base::flat_set<base::StringPiece>>
      kTrackableSecurityHeaders(base::flat_set<base::StringPiece>{
          "Strict-Transport-Security", "Expect-CT", "Public-Key-Pins",
          "Public-Key-Pins-Report-Only"});
  return kTrackableSecurityHeaders.get();
}

void RemoveTrackableSecurityHeadersForThirdParty(
    const GURL& request_url, const url::Origin& top_frame_origin,
    const net::HttpResponseHeaders* original_response_headers,
    scoped_refptr<net::HttpResponseHeaders>* override_response_headers) {
  if (!original_response_headers && !override_response_headers->get()) {
    return;
  }

  if (net::registry_controlled_domains::SameDomainOrHost(
          request_url, top_frame_origin,
          net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES)) {
    return;
  }

  if (!override_response_headers->get()) {
    *override_response_headers =
        new net::HttpResponseHeaders(original_response_headers->raw_headers());
  }
  for (auto header : *TrackableSecurityHeaders()) {
    (*override_response_headers)->RemoveHeader(std::string(header));
  }
}

}  // namespace brave
