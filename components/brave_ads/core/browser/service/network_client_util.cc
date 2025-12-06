/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/browser/service/network_client_util.h"

#include "brave/components/brave_ads/core/browser/service/oblivious_http_constants.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "url/gurl.h"

namespace brave_ads {

GURL ObliviousHttpKeyConfigUrl(bool use_staging) {
  return GURL(use_staging ? kStagingObliviousHttpKeyConfigUrl
                          : kProductionObliviousHttpKeyConfigUrl);
}

GURL ObliviousHttpRelayUrl(bool use_staging) {
  return GURL(use_staging ? kStagingObliviousHttpRelayUrl
                          : kProductionObliviousHttpRelayUrl);
}

net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation("brave_ads", R"(
      semantics {
        sender: "Brave Ads"
        description:
          "This service is used to communicate with Brave servers "
          "to send and retrieve information for Ads."
        trigger:
          "Triggered by user viewing ads or at various intervals."
        data:
          "Ads catalog and Confirmations."
        destination: WEBSITE
      }
      policy {
        cookies_allowed: NO
        setting:
          "You can enable or disable this feature by visiting brave://rewards."
        policy_exception_justification:
          "Not implemented."
      }
    )");
}

}  // namespace brave_ads
