/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/privacy/tokens/payment_tokens/payment_token_info.h"

#include <tuple>

namespace brave_ads::privacy {

PaymentTokenInfo::PaymentTokenInfo() = default;

PaymentTokenInfo::PaymentTokenInfo(const PaymentTokenInfo& other) = default;

PaymentTokenInfo& PaymentTokenInfo::operator=(const PaymentTokenInfo& other) =
    default;

PaymentTokenInfo::PaymentTokenInfo(PaymentTokenInfo&& other) noexcept = default;

PaymentTokenInfo& PaymentTokenInfo::operator=(
    PaymentTokenInfo&& other) noexcept = default;

PaymentTokenInfo::~PaymentTokenInfo() = default;

bool PaymentTokenInfo::operator==(const PaymentTokenInfo& other) const {
  const auto tie = [](const PaymentTokenInfo& payment_token) {
    return std::tie(payment_token.transaction_id, payment_token.public_key,
                    payment_token.unblinded_token,
                    payment_token.confirmation_type, payment_token.ad_type);
  };

  return tie(*this) == tie(other);
}

bool PaymentTokenInfo::operator!=(const PaymentTokenInfo& other) const {
  return !(*this == other);
}

}  // namespace brave_ads::privacy
