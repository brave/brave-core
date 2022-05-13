/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_ISSUERS_ISSUERS_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_ISSUERS_ISSUERS_UTIL_H_

#include <string>

#include "bat/ads/internal/account/issuers/issuer_types.h"

namespace absl {
template <typename T>
class optional;
}  // namespace absl

namespace ads {

struct IssuerInfo;
struct IssuersInfo;

bool IsIssuerValid(const IssuerInfo& issuer);

void SetIssuers(const IssuersInfo& issuers);
IssuersInfo GetIssuers();

bool HasIssuersChanged(const IssuersInfo& issuers);

bool IssuerExistsForType(const IssuerType issuer_type);

absl::optional<IssuerInfo> GetIssuerForType(const IssuersInfo& issuers,
                                            const IssuerType issuer_type);

bool PublicKeyExistsForIssuerType(const IssuerType issuer_type,
                                  const std::string& public_key);

absl::optional<double> GetSmallestNonZeroDenominationForIssuerType(
    const IssuerType issuer_type);

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_ISSUERS_ISSUERS_UTIL_H_
