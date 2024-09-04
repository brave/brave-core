/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_TOKENS_PAYMENT_TOKENS_PAYMENT_TOKENS_TEST_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_TOKENS_PAYMENT_TOKENS_PAYMENT_TOKENS_TEST_UTIL_H_

#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_token_info.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"

namespace brave_ads {

class PaymentTokens;

namespace test {

PaymentTokens& GetPaymentTokens();
PaymentTokenList SetPaymentTokens(int count);

PaymentTokenInfo BuildPaymentToken(
    mojom::ConfirmationType mojom_confirmation_type,
    mojom::AdType mojom_ad_type);
PaymentTokenInfo BuildPaymentToken();
PaymentTokenList BuildPaymentTokens(int count);

}  // namespace test

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_TOKENS_PAYMENT_TOKENS_PAYMENT_TOKENS_TEST_UTIL_H_
