/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_ISSUERS_ISSUERS_TEST_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_ISSUERS_ISSUERS_TEST_UTIL_H_

#include <string>

#include "brave/components/brave_ads/core/internal/account/issuers/token_issuers/token_issuer_info.h"

namespace brave_ads {

struct IssuersInfo;

namespace test {

TokenIssuerList BuildTokenIssuers();

std::string BuildIssuersUrlResponseBody();

IssuersInfo BuildIssuers(
    int ping,
    const TokenIssuerPublicKeyMap& confirmation_token_issuer_public_keys,
    const TokenIssuerPublicKeyMap& payment_token_issuer_public_keys);
IssuersInfo BuildIssuers();

void BuildAndSetIssuers();

}  // namespace test

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_ISSUERS_ISSUERS_TEST_UTIL_H_
