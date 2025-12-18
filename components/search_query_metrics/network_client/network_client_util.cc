/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/search_query_metrics/network_client/network_client_util.h"

#include "brave/components/search_query_metrics/network_client/oblivious_http_constants.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "url/gurl.h"

namespace metrics {

GURL ObliviousHttpKeyConfigUrl(bool use_staging) {
  return GURL(use_staging ? kStagingObliviousHttpKeyConfigUrl
                          : kProductionObliviousHttpKeyConfigUrl);
}

GURL ObliviousHttpRelayUrl(bool use_staging) {
  return GURL(use_staging ? kStagingObliviousHttpRelayUrl
                          : kProductionObliviousHttpRelayUrl);
}

net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation("search_query_metrics", R"(
      semantics {
        sender: "Search Query Metrics Service"
        description:
          "Sends a lightweight analytics ping when a user views search results "
          "in the Brave browser. The purpose is to measure aggregate search "
          "activity and feature usage in a privacy-preserving way."
        trigger:
          "Triggered when a user views a search results page in the browser."
        data:
          "A small payload of predefined key-value pairs describing search "
          "context and feature usage. All values are selected from a limited "
          "set of enumerated options to reduce fingerprinting risk. No search "
          "queries, URLs, personal identifiers are included."
        destination: WEBSITE
      }
      policy {
        cookies_allowed: NO
        setting:
          "This feature can be disabled by turning off the daily usage ping in "
          "settings."
        policy_exception_justification:
          "Not applicable. The request contains no personal data, does not use "
          "cookies, and is transmitted using Oblivious HTTP (OHTTP) to provide "
          "network-level unlinkability."
      }
    )");
}

}  // namespace metrics
