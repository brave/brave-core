/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_API_PURCHASE_ENDPOINTS_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_API_PURCHASE_ENDPOINTS_H_

#include <optional>
#include <string>

#include "base/strings/strcat.h"
#include "base/values.h"
#include "brave/components/brave_account/endpoint_client/request_types.h"
#include "brave/components/brave_account/endpoint_client/response.h"
#include "brave/components/brave_vpn/browser/v2/api/endpoint_constants.h"
#include "brave/components/brave_vpn/browser/v2/api/error_body.h"
#include "brave/components/brave_vpn/browser/v2/api/raw_json_response_body.h"
#include "url/gurl.h"
#include "url/url_constants.h"

// Purchase/subscription validation endpoints all hit the fixed Guardian account
// host (kVpnHost).

namespace brave_vpn::v2::endpoints {

inline constexpr char kHeaderBravePaymentsEnvironment[] =
    "Brave-Payments-Environment";

// GetSubscriberCredential API (legacy, store-purchase based) exchanges store
// purchase details for a subscriber credential. Shares the success/error bodies
// and URL with the V12 flow; only the request body differs.
struct GetSubscriberCredentialRequestBody {
  std::string product_type;
  std::string product_id;
  std::string validation_method;
  std::string purchase_token;
  std::string bundle_id;

  base::DictValue ToValue() const {
    return base::DictValue()
        .Set(kProductTypeKey, product_type)
        .Set(kProductIdKey, product_id)
        .Set(kValidationMethodKey, validation_method)
        .Set(kPurchaseTokenKey, purchase_token)
        .Set(kBundleIdKey, bundle_id);
  }
};

// GetSubscriberCredentialV12 API exchanges a SKUs credential for a subscriber
// credential.
struct GetSubscriberCredentialV12RequestBody {
  std::string skus_credential;

  base::DictValue ToValue() const {
    return base::DictValue()
        .Set(kValidationMethodKey, kValidationMethodDefaultValue)
        .Set(kSkusCredentialKey, skus_credential);
  }
};

struct GetSubscriberCredentialSuccessBody {
  std::string subscriber_credential;

  static std::optional<GetSubscriberCredentialSuccessBody> FromValue(
      const base::Value& value) {
    const auto* dict = value.GetIfDict();
    if (!dict) {
      return std::nullopt;
    }
    const auto* credential = dict->FindString(kSubscriberCredentialKey);
    if (!credential) {
      return std::nullopt;
    }
    return GetSubscriberCredentialSuccessBody{.subscriber_credential =
                                                  *credential};
  }

  base::DictValue ToValue() const {
    return base::DictValue().Set(kSubscriberCredentialKey,
                                 subscriber_credential);
  }
};

template <typename RequestBody>
struct GetSubscriberCredentialBase {
  using Request = brave_account::endpoint_client::POST<RequestBody>;
  using Response = brave_account::endpoint_client::
      Response<GetSubscriberCredentialSuccessBody, VpnErrorBody>;

  static GURL URL() {
    return GURL(base::StrCat({url::kHttpsScheme, url::kStandardSchemeSeparator,
                              kVpnHost}))
        .Resolve(kCreateSubscriberCredentialApi);
  }
};

using GetSubscriberCredential =
    GetSubscriberCredentialBase<GetSubscriberCredentialRequestBody>;

using GetSubscriberCredentialV12 =
    GetSubscriberCredentialBase<GetSubscriberCredentialV12RequestBody>;

// VerifyPurchaseToken API verifies a store purchase token and returns the
// server's JSON response verbatim.
struct VerifyPurchaseTokenRequestBody {
  std::string purchase_token;
  std::string product_id;
  std::string product_type;
  std::string bundle_id;

  base::DictValue ToValue() const {
    return base::DictValue()
        .Set(kPurchaseTokenKey, purchase_token)
        .Set(kProductIdKey, product_id)
        .Set(kProductTypeKey, product_type)
        .Set(kBundleIdKey, bundle_id);
  }
};

struct VerifyPurchaseToken {
  using Request =
      brave_account::endpoint_client::POST<VerifyPurchaseTokenRequestBody>;
  using Response =
      brave_account::endpoint_client::Response<RawJsonResponseBody,
                                               RawJsonResponseBody>;

  static GURL URL() {
    return GURL(base::StrCat({url::kHttpsScheme, url::kStandardSchemeSeparator,
                              kVpnHost}))
        .Resolve(kVerifyPurchaseTokenApi);
  }
};

}  // namespace brave_vpn::v2::endpoints

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_API_PURCHASE_ENDPOINTS_H_
