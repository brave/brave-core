/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_API_ENDPOINT_CONSTANTS_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_API_ENDPOINT_CONSTANTS_H_

namespace brave_vpn::v2::endpoints {

// Guardian Connect API host name.
inline constexpr char kVpnHost[] = "connect-api.guardianapp.com";

// Endpoint request APIs.
inline constexpr char kCreateSubscriberCredentialApi[] =
    "api/v1.2/subscriber-credential/create";
inline constexpr char kVerifyPurchaseTokenApi[] =
    "api/v1.1/verify-purchase-token";
inline constexpr char kCreateSupportTicketApi[] =
    "api/v1.2/partners/support-ticket";

// Endpoint request JSON keys that can be shared between multiple APIs.
inline constexpr char kProductTypeKey[] = "product-type";
inline constexpr char kProductIdKey[] = "product-id";
inline constexpr char kPurchaseTokenKey[] = "purchase-token";
inline constexpr char kBundleIdKey[] = "bundle-id";
inline constexpr char kValidationMethodKey[] = "validation-method";
inline constexpr char kSkusCredentialKey[] = "brave-vpn-premium-monthly-pass";
inline constexpr char kSubscriberCredentialKey[] = "subscriber-credential";
inline constexpr char kEmailKey[] = "email";
inline constexpr char kSubjectKey[] = "subject";
inline constexpr char kSupportTicketKey[] = "support-ticket";
inline constexpr char kPartnerClientIdKey[] = "partner-client-id";
inline constexpr char kPaymentValidationMethodKey[] =
    "payment-validation-method";

// Default payment validation method used by Brave.
inline constexpr char kValidationMethodDefaultValue[] = "brave-premium";

}  // namespace brave_vpn::v2::endpoints

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_API_ENDPOINT_CONSTANTS_H_
