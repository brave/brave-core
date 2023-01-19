/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_token_info.h"

namespace ads::privacy {

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
  return transaction_id == other.transaction_id &&
         public_key == other.public_key && value == other.value &&
         confirmation_type == other.confirmation_type &&
         ad_type == other.ad_type;
}

bool UnblindedPaymentTokenInfo::operator!=(
    const UnblindedPaymentTokenInfo& other) const {
  return !(*this == other);
}

}  // namespace ads::privacy
