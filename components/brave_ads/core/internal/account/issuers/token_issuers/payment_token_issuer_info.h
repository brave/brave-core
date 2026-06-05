/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_ISSUERS_TOKEN_ISSUERS_PAYMENT_TOKEN_ISSUER_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_ISSUERS_TOKEN_ISSUERS_PAYMENT_TOKEN_ISSUER_INFO_H_

#include <string>

#include "base/containers/flat_map.h"

namespace brave_ads {

using PaymentTokenIssuerPublicKeyMap =
    base::flat_map</*public_key*/ std::string, /*bucket_value*/ double>;

struct PaymentTokenIssuerInfo final {
  PaymentTokenIssuerInfo();

  PaymentTokenIssuerInfo(const PaymentTokenIssuerInfo&);
  PaymentTokenIssuerInfo& operator=(const PaymentTokenIssuerInfo&);

  PaymentTokenIssuerInfo(PaymentTokenIssuerInfo&&) noexcept;
  PaymentTokenIssuerInfo& operator=(PaymentTokenIssuerInfo&&) noexcept;

  ~PaymentTokenIssuerInfo();

  bool operator==(const PaymentTokenIssuerInfo&) const = default;

  PaymentTokenIssuerPublicKeyMap public_keys;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_ISSUERS_TOKEN_ISSUERS_PAYMENT_TOKEN_ISSUER_INFO_H_
