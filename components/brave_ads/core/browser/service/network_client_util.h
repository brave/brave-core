/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_SERVICE_NETWORK_CLIENT_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_SERVICE_NETWORK_CLIENT_UTIL_H_

#include <optional>
#include <string>
#include <string_view>

#include "base/containers/flat_map.h"
#include "base/memory/scoped_refptr.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"

namespace net {
class HttpResponseHeaders;
struct NetworkTrafficAnnotationTag;
}  // namespace net

namespace brave_ads {

// Converts `mojom::UrlRequestMethodType` to its string representation.
std::string ToString(mojom::UrlRequestMethodType value);

// Returns the network traffic annotation tag for network requests.
net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTag();

// Extracts all HTTP response headers from a `net::HttpResponseHeaders` object
// and returns them as a flat map with lowercased keys.
base::flat_map<std::string, std::string> ExtractHttpResponseHeaders(
    const scoped_refptr<net::HttpResponseHeaders>& http_response_headers);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_SERVICE_NETWORK_CLIENT_UTIL_H_
