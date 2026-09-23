/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_API_SUPPORT_ENDPOINTS_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_API_SUPPORT_ENDPOINTS_H_

#include <string>

#include "base/base64.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "brave/components/brave_account/endpoint_client/request_types.h"
#include "brave/components/brave_account/endpoint_client/response.h"
#include "brave/components/brave_vpn/browser/v2/api/endpoint_constants.h"
#include "brave/components/brave_vpn/browser/v2/api/error_body.h"
#include "brave/components/brave_vpn/browser/v2/api/raw_json_response_body.h"
#include "url/gurl.h"
#include "url/url_constants.h"

// CreateSupportTicket hits the fixed Guardian account host (kVpnHost).

namespace brave_vpn::v2::endpoints {

inline constexpr char kPartnerClientIdValue[] = "com.brave.browser";
inline constexpr char kTimezoneMetadataKey[] = "timezone";

// CreateSupportTicket API submits a customer support inquiry to Guardian.
// On success (200), the request JSON is echoed back as confirmation; since it
// carries no new information, it's forwarded verbatim via RawJsonResponseBody
// rather than parsed into fields.
struct CreateSupportTicketRequestBody {
  std::string email;
  std::string subject;
  std::string body;
  std::string subscriber_credential;
  std::string timezone;

  base::DictValue ToValue() const {
    const std::string body_with_metadata = base::StrCat(
        {body, "\n\n", kSubscriberCredentialKey, ": ", subscriber_credential,
         "\n", kPaymentValidationMethodKey, ": ", kValidationMethodDefaultValue,
         "\n", kTimezoneMetadataKey, ": ", timezone});

    std::string email_trimmed, subject_trimmed, body_trimmed;
    base::TrimWhitespaceASCII(email, base::TRIM_ALL, &email_trimmed);
    base::TrimWhitespaceASCII(subject, base::TRIM_ALL, &subject_trimmed);
    base::TrimWhitespaceASCII(body_with_metadata, base::TRIM_ALL,
                              &body_trimmed);

    return base::DictValue()
        .Set(kEmailKey, email_trimmed)
        .Set(kSubjectKey, subject_trimmed)
        .Set(kSupportTicketKey, base::Base64Encode(body_trimmed))
        .Set(kPartnerClientIdKey, kPartnerClientIdValue)
        .Set(kPaymentValidationMethodKey, kValidationMethodDefaultValue)
        .Set(kSubscriberCredentialKey, subscriber_credential);
  }
};

struct CreateSupportTicket {
  using Request =
      brave_account::endpoint_client::POST<CreateSupportTicketRequestBody>;
  using Response = brave_account::endpoint_client::Response<RawJsonResponseBody,
                                                            VpnErrorBody>;

  static GURL URL() {
    return GURL(base::StrCat({url::kHttpsScheme, url::kStandardSchemeSeparator,
                              kVpnHost}))
        .Resolve(kCreateSupportTicketApi);
  }
};

}  // namespace brave_vpn::v2::endpoints

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_API_SUPPORT_ENDPOINTS_H_
