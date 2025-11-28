/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_SERVICE_NETWORK_CLIENT_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_SERVICE_NETWORK_CLIENT_UTIL_H_

#include <string>

#include "base/containers/flat_map.h"
#include "base/memory/scoped_refptr.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"
#include "services/network/public/mojom/oblivious_http_request.mojom-forward.h"

class GURL;

namespace net {
class HttpResponseHeaders;
struct NetworkTrafficAnnotationTag;
}  // namespace net

namespace brave_ads {

// Returns the OHTTP key config URL corresponding to the staging or production
// environment, depending on `use_staging`.
GURL ObliviousHttpKeyConfigUrl(bool use_staging);

// Returns the OHTTP relay URL corresponding to the staging or production
// environment, depending on `use_staging`.
GURL ObliviousHttpRelayUrl(bool use_staging);

// Converts `mojom::UrlRequestMethodType` to its string representation.
std::string ToString(mojom::UrlRequestMethodType value);

// Returns the network traffic annotation tag used to identify and audit network
// requests.
net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTag();

// Builds an `network::mojom::ObliviousHttpRequest` from the given parameters.
network::mojom::ObliviousHttpRequestPtr BuildObliviousHttpRequest(
    const GURL& relay_url,
    const std::string& key_config,
    const mojom::UrlRequestInfoPtr& mojom_url_request);

// Extracts all HTTP response headers from `net::HttpResponseHeaders` and
// returns them as a flat map with lowercased keys.
base::flat_map<std::string, std::string> ExtractHttpResponseHeaders(
    const scoped_refptr<net::HttpResponseHeaders>& http_response_headers);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_SERVICE_NETWORK_CLIENT_UTIL_H_
