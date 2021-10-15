/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/public/common/client_hints/enabled_client_hints.h"

#define SetIsEnabled SetIsEnabled_Unused

#include "../../../../../../third_party/blink/common/client_hints/enabled_client_hints.cc"

#undef SetIsEnabled

namespace blink {

void EnabledClientHints::SetIsEnabled(const WebClientHintsType type,
                                      const bool should_send) {
  enabled_types_[static_cast<int>(type)] = false;
}

void EnabledClientHints::SetIsEnabled(
    const GURL& url,
    const GURL* third_party_url,
    const net::HttpResponseHeaders* response_headers,
    const network::mojom::WebClientHintsType type,
    const bool should_send) {
  SetIsEnabled(type, false);
}

}  // namespace blink
