/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_TOKENS_PAYMENT_TOKENS_PAYMENT_TOKEN_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_TOKENS_PAYMENT_TOKENS_PAYMENT_TOKEN_UTIL_H_

#include <cstddef>
#include <optional>

#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_token_info.h"

namespace brave_ads {

std::optional<PaymentTokenInfo> MaybeGetPaymentToken();

const PaymentTokenList& GetAllPaymentTokens();

void AddPaymentTokens(const PaymentTokenList& payment_tokens);

bool RemovePaymentToken(const PaymentTokenInfo& payment_token);
void RemovePaymentTokens(const PaymentTokenList& payment_tokens);
void RemoveAllPaymentTokens();

bool PaymentTokenExists(const PaymentTokenInfo& payment_token);

bool PaymentTokensIsEmpty();

size_t PaymentTokenCount();

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_TOKENS_PAYMENT_TOKENS_PAYMENT_TOKEN_UTIL_H_
