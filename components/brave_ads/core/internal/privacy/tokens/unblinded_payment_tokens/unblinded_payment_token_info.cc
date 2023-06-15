/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_token_info.h"

#include <tuple>

namespace brave_ads::privacy {

UnblindedPaymentTokenInfo::UnblindedPaymentTokenInfo() = default;

UnblindedPaymentTokenInfo::UnblindedPaymentTokenInfo(
    const UnblindedPaymentTokenInfo& other) = default;

UnblindedPaymentTokenInfo& UnblindedPaymentTokenInfo::operator=(
    const UnblindedPaymentTokenInfo& other) = default;

UnblindedPaymentTokenInfo::UnblindedPaymentTokenInfo(
    UnblindedPaymentTokenInfo&& other) noexcept = default;

UnblindedPaymentTokenInfo& UnblindedPaymentTokenInfo::operator=(
    UnblindedPaymentTokenInfo&& other) noexcept = default;

UnblindedPaymentTokenInfo::~UnblindedPaymentTokenInfo() = default;

bool UnblindedPaymentTokenInfo::operator==(
    const UnblindedPaymentTokenInfo& other) const {
  const auto tie =
      [](const UnblindedPaymentTokenInfo& unblinded_payment_token) {
        return std::tie(unblinded_payment_token.transaction_id,
                        unblinded_payment_token.public_key,
                        unblinded_payment_token.value,
                        unblinded_payment_token.confirmation_type,
                        unblinded_payment_token.ad_type);
      };

  return tie(*this) == tie(other);
}

bool UnblindedPaymentTokenInfo::operator!=(
    const UnblindedPaymentTokenInfo& other) const {
  return !(*this == other);
}

}  // namespace brave_ads::privacy
