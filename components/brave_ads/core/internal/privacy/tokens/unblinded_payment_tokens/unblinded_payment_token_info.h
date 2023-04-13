/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PRIVACY_TOKENS_UNBLINDED_PAYMENT_TOKENS_UNBLINDED_PAYMENT_TOKEN_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PRIVACY_TOKENS_UNBLINDED_PAYMENT_TOKENS_UNBLINDED_PAYMENT_TOKEN_INFO_H_

#include <string>
#include <vector>

#include "brave/components/brave_ads/core/ad_type.h"
#include "brave/components/brave_ads/core/confirmation_type.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/public_key.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/unblinded_token.h"

namespace brave_ads::privacy {

struct UnblindedPaymentTokenInfo final {
  UnblindedPaymentTokenInfo();

  UnblindedPaymentTokenInfo(const UnblindedPaymentTokenInfo&);
  UnblindedPaymentTokenInfo& operator=(const UnblindedPaymentTokenInfo&);

  UnblindedPaymentTokenInfo(UnblindedPaymentTokenInfo&&) noexcept;
  UnblindedPaymentTokenInfo& operator=(UnblindedPaymentTokenInfo&&) noexcept;

  ~UnblindedPaymentTokenInfo();

  bool operator==(const UnblindedPaymentTokenInfo&) const;
  bool operator!=(const UnblindedPaymentTokenInfo&) const;

  std::string transaction_id;
  cbr::UnblindedToken value;
  cbr::PublicKey public_key;
  ConfirmationType confirmation_type = ConfirmationType::kUndefined;
  AdType ad_type = AdType::kUndefined;
};

using UnblindedPaymentTokenList = std::vector<UnblindedPaymentTokenInfo>;

}  // namespace brave_ads::privacy

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PRIVACY_TOKENS_UNBLINDED_PAYMENT_TOKENS_UNBLINDED_PAYMENT_TOKEN_INFO_H_
