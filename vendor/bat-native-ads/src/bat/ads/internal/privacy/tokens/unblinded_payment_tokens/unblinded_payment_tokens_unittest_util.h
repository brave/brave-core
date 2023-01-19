/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_TOKENS_UNBLINDED_PAYMENT_TOKENS_UNBLINDED_PAYMENT_TOKENS_UNITTEST_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_TOKENS_UNBLINDED_PAYMENT_TOKENS_UNBLINDED_PAYMENT_TOKENS_UNITTEST_UTIL_H_

#include <string>
#include <vector>

#include "bat/ads/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_token_info.h"

namespace ads::privacy {

class UnblindedPaymentTokens;

UnblindedPaymentTokens* GetUnblindedPaymentTokens();

UnblindedPaymentTokenList SetUnblindedPaymentTokens(int count);

UnblindedPaymentTokenInfo CreateUnblindedPaymentToken(
    const std::string& unblinded_payment_token_base64);
UnblindedPaymentTokenInfo CreateUnblindedPaymentToken(
    const ConfirmationType& confirmation_type,
    const AdType& ad_type);
UnblindedPaymentTokenList CreateUnblindedPaymentTokens(
    const std::vector<std::string>& unblinded_payment_tokens_base64);

UnblindedPaymentTokenList GetUnblindedPaymentTokens(int count);
UnblindedPaymentTokenInfo GetUnblindedPaymentToken();

}  // namespace ads::privacy

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_TOKENS_UNBLINDED_PAYMENT_TOKENS_UNBLINDED_PAYMENT_TOKENS_UNITTEST_UTIL_H_
