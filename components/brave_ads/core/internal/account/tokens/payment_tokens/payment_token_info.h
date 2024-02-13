/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_TOKENS_PAYMENT_TOKENS_PAYMENT_TOKEN_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_TOKENS_PAYMENT_TOKENS_PAYMENT_TOKEN_INFO_H_

#include <string>
#include <vector>

#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/public_key.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/unblinded_token.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/public/ad_units/ad_type.h"

namespace brave_ads {

struct PaymentTokenInfo final {
  PaymentTokenInfo();

  PaymentTokenInfo(const PaymentTokenInfo&);
  PaymentTokenInfo& operator=(const PaymentTokenInfo&);

  PaymentTokenInfo(PaymentTokenInfo&&) noexcept;
  PaymentTokenInfo& operator=(PaymentTokenInfo&&) noexcept;

  ~PaymentTokenInfo();

  bool operator==(const PaymentTokenInfo&) const = default;

  std::string transaction_id;
  cbr::UnblindedToken unblinded_token;
  cbr::PublicKey public_key;
  ConfirmationType confirmation_type = ConfirmationType::kUndefined;
  AdType ad_type = AdType::kUndefined;
};

using PaymentTokenList = std::vector<PaymentTokenInfo>;

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_TOKENS_PAYMENT_TOKENS_PAYMENT_TOKEN_INFO_H_
