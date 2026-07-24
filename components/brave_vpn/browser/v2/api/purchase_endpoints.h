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
#include "brave/components/brave_vpn/browser/v2/api/error_body.h"
#include "brave/components/brave_vpn/common/brave_vpn_constants.h"
#include "url/gurl.h"
#include "url/url_constants.h"

// Purchase/subscription validation endpoints all hit the fixed Guardian account
// host (kVpnHost).

namespace brave_vpn::v2::endpoints {

// GetSubscriberCredentialV12 API exchanges a SKUs credential for a subscriber
// credential.
struct GetSubscriberCredentialV12RequestBody {
  std::string validation_method = "brave-premium";
  std::string skus_credential;

  base::DictValue ToValue() const {
    return base::DictValue()
        .Set("validation-method", validation_method)
        .Set("brave-vpn-premium-monthly-pass", skus_credential);
  }
};

struct GetSubscriberCredentialV12ResponseBody {
  std::string subscriber_credential;

  static std::optional<GetSubscriberCredentialV12ResponseBody> FromValue(
      const base::Value& value) {
    const auto* dict = value.GetIfDict();
    if (!dict) {
      return std::nullopt;
    }
    const auto* credential = dict->FindString("subscriber-credential");
    if (!credential) {
      return std::nullopt;
    }
    return GetSubscriberCredentialV12ResponseBody{.subscriber_credential =
                                                      *credential};
  }
};

struct GetSubscriberCredentialV12 {
  using Request = brave_account::endpoint_client::POST<
      GetSubscriberCredentialV12RequestBody>;
  using Response = brave_account::endpoint_client::
      Response<GetSubscriberCredentialV12ResponseBody, VpnErrorBody>;

  static GURL URL() {
    return GURL(base::StrCat({url::kHttpsScheme, url::kStandardSchemeSeparator,
                              kVpnHost}))
        .Resolve(kCreateSubscriberCredentialV12);
  }
};

}  // namespace brave_vpn::v2::endpoints

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_API_PURCHASE_ENDPOINTS_H_
