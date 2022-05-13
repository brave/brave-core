/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/privacy/unblinded_payment_tokens/unblinded_payment_token_info.h"

namespace ads {
namespace privacy {

UnblindedPaymentTokenInfo::UnblindedPaymentTokenInfo()
    : value(nullptr), public_key(nullptr) {}

UnblindedPaymentTokenInfo::UnblindedPaymentTokenInfo(
    const UnblindedPaymentTokenInfo& unblinded_payment_token) = default;

UnblindedPaymentTokenInfo::~UnblindedPaymentTokenInfo() = default;

bool UnblindedPaymentTokenInfo::operator==(
    const UnblindedPaymentTokenInfo& rhs) const {
  return transaction_id == rhs.transaction_id && public_key == rhs.public_key &&
         value == rhs.value && confirmation_type == rhs.confirmation_type &&
         ad_type == rhs.ad_type;
}

bool UnblindedPaymentTokenInfo::operator!=(
    const UnblindedPaymentTokenInfo& rhs) const {
  return !(*this == rhs);
}

}  // namespace privacy
}  // namespace ads
