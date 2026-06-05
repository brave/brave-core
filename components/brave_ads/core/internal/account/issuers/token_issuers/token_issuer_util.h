/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_ISSUERS_TOKEN_ISSUERS_TOKEN_ISSUER_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_ISSUERS_TOKEN_ISSUERS_TOKEN_ISSUER_UTIL_H_

namespace brave_ads {

namespace cbr {
class PublicKey;
}  // namespace cbr

bool ConfirmationTokenIssuerExists();
bool PaymentTokenIssuerExists();

bool ConfirmationTokenIssuerPublicKeyExists(const cbr::PublicKey& public_key);
bool PaymentTokenIssuerPublicKeyExists(const cbr::PublicKey& public_key);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_ISSUERS_TOKEN_ISSUERS_TOKEN_ISSUER_UTIL_H_
