/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_API_REGION_ENDPOINTS_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_API_REGION_ENDPOINTS_H_

#include <string>

#include "base/strings/strcat.h"
#include "base/values.h"
#include "brave/components/brave_account/endpoint_client/request_types.h"
#include "brave/components/brave_account/endpoint_client/response.h"
#include "brave/components/brave_vpn/browser/v2/api/empty_request_body.h"
#include "brave/components/brave_vpn/browser/v2/api/endpoint_constants.h"
#include "brave/components/brave_vpn/browser/v2/api/error_body.h"
#include "brave/components/brave_vpn/browser/v2/api/raw_json_response_body.h"
#include "url/gurl.h"
#include "url/url_constants.h"

// Region-discovery endpoints all hit the fixed Guardian account host
// (kVpnHost). Responses are forwarded verbatim (RawJsonResponseBody) rather
// than parsed into fields - typed interpretation belongs to higher-level
// components, not API layer.

namespace brave_vpn::v2::endpoints {

// GetServerRegions API returns all regions at a given precision which is a URL
// path segment and is appended by callers.
struct GetServerRegions {
  using Request = brave_account::endpoint_client::GET<EmptyRequestBody>;
  using Response = brave_account::endpoint_client::Response<RawJsonResponseBody,
                                                            VpnErrorBody>;

  static GURL URL() {
    return GURL(base::StrCat({url::kHttpsScheme, url::kStandardSchemeSeparator,
                              kVpnHost}))
        .Resolve(kAllServerRegionsApi);
  }
};

// GetTimezonesForRegions API returns timezone metadata for all regions.
struct GetTimezonesForRegions {
  using Request = brave_account::endpoint_client::GET<EmptyRequestBody>;
  using Response = brave_account::endpoint_client::Response<RawJsonResponseBody,
                                                            VpnErrorBody>;

  static GURL URL() {
    return GURL(base::StrCat({url::kHttpsScheme, url::kStandardSchemeSeparator,
                              kVpnHost}))
        .Resolve(kTimezonesForRegionsApi);
  }
};

// GetHostnamesForRegion API returns the SGW servers available for a region.
struct GetHostnamesForRegionRequestBody {
  std::string region;
  std::string region_precision;

  base::DictValue ToValue() const {
    return base::DictValue()
        .Set(kRegionKey, region)
        .Set(kRegionPrecisionKey, region_precision);
  }
};

struct GetHostnamesForRegion {
  using Request =
      brave_account::endpoint_client::POST<GetHostnamesForRegionRequestBody>;
  using Response = brave_account::endpoint_client::Response<RawJsonResponseBody,
                                                            VpnErrorBody>;

  static GURL URL() {
    return GURL(base::StrCat({url::kHttpsScheme, url::kStandardSchemeSeparator,
                              kVpnHost}))
        .Resolve(kHostnamesForRegionApi);
  }
};

}  // namespace brave_vpn::v2::endpoints

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_API_REGION_ENDPOINTS_H_
