/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_API_DEVICE_ENDPOINTS_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_API_DEVICE_ENDPOINTS_H_

#include <optional>
#include <string>

#include "base/strings/strcat.h"
#include "base/values.h"
#include "brave/components/brave_account/endpoint_client/request_types.h"
#include "brave/components/brave_account/endpoint_client/response.h"
#include "brave/components/brave_vpn/browser/v2/api/empty_request_body.h"
#include "brave/components/brave_vpn/browser/v2/api/endpoint_constants.h"
#include "brave/components/brave_vpn/browser/v2/api/error_body.h"
#include "brave/components/brave_vpn/browser/v2/api/raw_json_response_body.h"
#include "brave/components/brave_vpn/browser/v2/api/transport_protocol.h"
#include "url/gurl.h"
#include "url/url_constants.h"

// SGW endpoints, unlike the Connect API endpoints, target a caller-supplied VPN
// node, not a fixed host, so URL() resolves against a placeholder that every
// client method must override via UrlReplacements::SetHost() before sending.

namespace brave_vpn::v2::endpoints {

// Reserved per RFC 2606; never resolved. Every endpoint below must have its
// host overridden via UrlReplacements::SetHost().
inline constexpr char kDeviceHostPlaceholder[] = "device.invalid";

// Guardian authenticates bodyless GET calls via this request header instead.
inline constexpr char kHeaderGrdApiAuthToken[] = "grd-api-auth-token";

// GetProfileCredentials API registers VPN credentials with an SGW node.
// |public_key| is only required and sent for WireGuard protocol.
// |multihop_exit_region| is optional and omitted when not set.
struct GetProfileCredentialsRequestBody {
  std::string subscriber_credential;
  TransportProtocol transport_protocol = TransportProtocol::kIKEv2;
  std::string public_key;
  std::optional<std::string> multihop_exit_region;

  base::DictValue ToValue() const {
    base::DictValue dict;
    dict.Set(kSubscriberCredentialKey, subscriber_credential);
    dict.Set(kTransportProtocolKey,
             ToTransportProtocolString(transport_protocol));
    if (transport_protocol == TransportProtocol::kWireguard) {
      dict.Set(kPublicKeyKey, public_key);
    }
    if (multihop_exit_region.has_value()) {
      dict.Set(kMultihopExitRegionKey, multihop_exit_region.value());
    }
    return dict;
  }
};

struct GetProfileCredentials {
  using Request =
      brave_account::endpoint_client::POST<GetProfileCredentialsRequestBody>;
  using Response = brave_account::endpoint_client::Response<RawJsonResponseBody,
                                                            VpnErrorBody>;

  static GURL URL() {
    return GURL(base::StrCat({url::kHttpsScheme, url::kStandardSchemeSeparator,
                              kDeviceHostPlaceholder}))
        .Resolve(kDeviceCredentialsApi);
  }
};

// VerifyCredentials API confirms a VPN credential (identified by a client-id
// path segment and an api auth token) has not been invalidated.
struct VerifyCredentials {
  using Request = brave_account::endpoint_client::GET<EmptyRequestBody>;
  using Response = brave_account::endpoint_client::Response<RawJsonResponseBody,
                                                            VpnErrorBody>;

  static GURL URL() {
    return GURL(base::StrCat({url::kHttpsScheme, url::kStandardSchemeSeparator,
                              kDeviceHostPlaceholder}))
        .Resolve(kDeviceApi);
  }
};

// InvalidateCredentials API explicitly disables a VPN credential (identified
// by a client-id path segment and an api auth token). This is a destructive
// action and should be confirmed by sending a subscriber credential too.
struct InvalidateCredentialsRequestBody {
  std::string api_auth_token;
  std::string subscriber_credential;

  base::DictValue ToValue() const {
    return base::DictValue()
        .Set(kApiAuthTokenKey, api_auth_token)
        .Set(kSubscriberCredentialKey, subscriber_credential);
  }
};

struct InvalidateCredentials {
  using Request =
      brave_account::endpoint_client::POST<InvalidateCredentialsRequestBody>;
  using Response = brave_account::endpoint_client::Response<RawJsonResponseBody,
                                                            VpnErrorBody>;

  static GURL URL() {
    return GURL(base::StrCat({url::kHttpsScheme, url::kStandardSchemeSeparator,
                              kDeviceHostPlaceholder}))
        .Resolve(kDeviceApi);
  }
};

// GetAvailableMultihopExitRegions API returns the currently configured multihop
// exit region and the destinations available to choose from (identified by a
// client-id path segment and an api auth token). Response is forwarded
// verbatim.
struct GetAvailableMultihopExitRegions {
  using Request = brave_account::endpoint_client::GET<EmptyRequestBody>;
  using Response = brave_account::endpoint_client::Response<RawJsonResponseBody,
                                                            VpnErrorBody>;

  static GURL URL() {
    return GURL(base::StrCat({url::kHttpsScheme, url::kStandardSchemeSeparator,
                              kDeviceHostPlaceholder}))
        .Resolve(kDeviceApi);
  }
};

// SetMultihopExitRegion API configures the multihop egress destination
// (identified by a client-id path segment and an api auth token).
struct SetMultihopExitRegionRequestBody {
  std::string api_auth_token;
  std::string multihop_exit_region;

  base::DictValue ToValue() const {
    return base::DictValue()
        .Set(kApiAuthTokenKey, api_auth_token)
        .Set(kMultihopExitRegionKey, multihop_exit_region);
  }
};

struct SetMultihopExitRegion {
  using Request =
      brave_account::endpoint_client::POST<SetMultihopExitRegionRequestBody>;
  using Response = brave_account::endpoint_client::Response<RawJsonResponseBody,
                                                            VpnErrorBody>;

  static GURL URL() {
    return GURL(base::StrCat({url::kHttpsScheme, url::kStandardSchemeSeparator,
                              kDeviceHostPlaceholder}))
        .Resolve(kDeviceApi);
  }
};

}  // namespace brave_vpn::v2::endpoints

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_API_DEVICE_ENDPOINTS_H_
