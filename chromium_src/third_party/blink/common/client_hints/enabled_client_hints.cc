/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/public/common/client_hints/enabled_client_hints.h"
#include "third_party/blink/public/common/features.h"

#define SetIsEnabled SetIsEnabled_ChromiumImpl

#include "src/third_party/blink/common/client_hints/enabled_client_hints.cc"

#undef SetIsEnabled

namespace blink {

void EnabledClientHints::SetIsEnabled(const WebClientHintsType type,
                                      const bool should_send) {
  switch (type) {
    case WebClientHintsType::kUA:
    case WebClientHintsType::kUAMobile:
    case WebClientHintsType::kUAPlatform:
      if (base::FeatureList::IsEnabled(
              blink::features::kAllowCertainClientHints)) {
        SetIsEnabled_ChromiumImpl(type, should_send);
        break;
      }
      ABSL_FALLTHROUGH_INTENDED;
    default:
      enabled_types_[static_cast<int>(type)] = false;
  }
}

void EnabledClientHints::SetIsEnabled(
    const GURL& url,
    const absl::optional<GURL>& third_party_url,
    const net::HttpResponseHeaders* response_headers,
    const network::mojom::WebClientHintsType type,
    const bool should_send) {
  switch (type) {
    case WebClientHintsType::kUA:
    case WebClientHintsType::kUAMobile:
    case WebClientHintsType::kUAPlatform:
      if (base::FeatureList::IsEnabled(
              blink::features::kAllowCertainClientHints)) {
        SetIsEnabled_ChromiumImpl(url, third_party_url, response_headers, type,
                                  should_send);
        break;
      }
      ABSL_FALLTHROUGH_INTENDED;
    default:
      SetIsEnabled(type, should_send);
  }
}

}  // namespace blink
