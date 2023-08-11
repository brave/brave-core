/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PRIVACY_TOKENS_PAYMENT_TOKENS_PAYMENT_TOKENS_UNITTEST_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PRIVACY_TOKENS_PAYMENT_TOKENS_PAYMENT_TOKENS_UNITTEST_UTIL_H_

#include <string>
#include <vector>

#include "brave/components/brave_ads/core/internal/privacy/tokens/payment_tokens/payment_token_info.h"

namespace brave_ads {

class AdType;

namespace privacy {

class PaymentTokens;

PaymentTokens& GetPaymentTokensForTesting();
PaymentTokenList SetPaymentTokensForTesting(int count);

PaymentTokenInfo BuildPaymentTokenForTesting(
    const ConfirmationType& confirmation_type,
    const AdType& ad_type);
PaymentTokenList BuildPaymentTokensForTesting(
    const std::vector<std::string>& payment_tokens_base64);
PaymentTokenInfo BuildPaymentTokenForTesting(
    const std::string& payment_token_base64);
PaymentTokenList BuildPaymentTokensForTesting(int count);
PaymentTokenInfo BuildPaymentTokenForTesting();

}  // namespace privacy
}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PRIVACY_TOKENS_PAYMENT_TOKENS_PAYMENT_TOKENS_UNITTEST_UTIL_H_
