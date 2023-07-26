/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PRIVACY_TOKENS_PAYMENT_TOKENS_PAYMENT_TOKEN_VALUE_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PRIVACY_TOKENS_PAYMENT_TOKENS_PAYMENT_TOKEN_VALUE_UTIL_H_

#include "brave/components/brave_ads/core/internal/privacy/tokens/payment_tokens/payment_token_info.h"

#include "base/values.h"

namespace brave_ads::privacy {

base::Value::List PaymentTokensToValue(const PaymentTokenList& payment_tokens);
PaymentTokenList PaymentTokensFromValue(const base::Value::List& list);

}  // namespace brave_ads::privacy

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PRIVACY_TOKENS_PAYMENT_TOKENS_PAYMENT_TOKEN_VALUE_UTIL_H_
