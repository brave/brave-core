/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_TOKENS_UNBLINDED_PAYMENT_TOKENS_UNBLINDED_PAYMENT_TOKEN_VALUE_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_TOKENS_UNBLINDED_PAYMENT_TOKENS_UNBLINDED_PAYMENT_TOKEN_VALUE_UTIL_H_

#include "bat/ads/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_token_info.h"

#include "base/values.h"

namespace ads::privacy {

base::Value::List UnblindedPaymentTokensToValue(
    const UnblindedPaymentTokenList& unblinded_tokens);

UnblindedPaymentTokenList UnblindedPaymentTokensFromValue(
    const base::Value::List& list);

}  // namespace ads::privacy

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_TOKENS_UNBLINDED_PAYMENT_TOKENS_UNBLINDED_PAYMENT_TOKEN_VALUE_UTIL_H_
