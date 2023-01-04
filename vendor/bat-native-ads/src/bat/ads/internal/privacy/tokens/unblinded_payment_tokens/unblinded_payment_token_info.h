/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_TOKENS_UNBLINDED_PAYMENT_TOKENS_UNBLINDED_PAYMENT_TOKEN_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_TOKENS_UNBLINDED_PAYMENT_TOKENS_UNBLINDED_PAYMENT_TOKEN_INFO_H_

#include <string>
#include <vector>

#include "bat/ads/ad_type.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/public_key.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/unblinded_token.h"

namespace ads::privacy {

struct UnblindedPaymentTokenInfo final {
  UnblindedPaymentTokenInfo();

  UnblindedPaymentTokenInfo(const UnblindedPaymentTokenInfo& other);
  UnblindedPaymentTokenInfo& operator=(const UnblindedPaymentTokenInfo& other);

  UnblindedPaymentTokenInfo(UnblindedPaymentTokenInfo&& other) noexcept;
  UnblindedPaymentTokenInfo& operator=(
      UnblindedPaymentTokenInfo&& other) noexcept;

  ~UnblindedPaymentTokenInfo();

  bool operator==(const UnblindedPaymentTokenInfo& other) const;
  bool operator!=(const UnblindedPaymentTokenInfo& other) const;

  std::string transaction_id;
  cbr::UnblindedToken value;
  cbr::PublicKey public_key;
  ConfirmationType confirmation_type = ConfirmationType::kUndefined;
  AdType ad_type = AdType::kUndefined;
};

using UnblindedPaymentTokenList = std::vector<UnblindedPaymentTokenInfo>;

}  // namespace ads::privacy

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_TOKENS_UNBLINDED_PAYMENT_TOKENS_UNBLINDED_PAYMENT_TOKEN_INFO_H_
