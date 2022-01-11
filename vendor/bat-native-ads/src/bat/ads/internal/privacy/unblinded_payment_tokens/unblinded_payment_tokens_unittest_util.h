/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_UNBLINDED_PAYMENT_TOKENS_UNBLINDED_PAYMENT_TOKENS_UNITTEST_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_UNBLINDED_PAYMENT_TOKENS_UNBLINDED_PAYMENT_TOKENS_UNITTEST_UTIL_H_

#include <string>
#include <vector>

#include "base/values.h"
#include "bat/ads/internal/privacy/unblinded_payment_tokens/unblinded_payment_token_info_aliases.h"

namespace ads {
namespace privacy {

class UnblindedPaymentTokens;
struct UnblindedPaymentTokenInfo;

UnblindedPaymentTokens* get_unblinded_payment_tokens();

UnblindedPaymentTokenList SetUnblindedPaymentTokens(const int count);

UnblindedPaymentTokenInfo CreateUnblindedPaymentToken(
    const std::string& unblinded_payment_token_base64);

UnblindedPaymentTokenList CreateUnblindedPaymentTokens(
    const std::vector<std::string>& unblinded_payment_tokens_base64);

UnblindedPaymentTokenList GetUnblindedPaymentTokens(const int count);

UnblindedPaymentTokenList GetRandomUnblindedPaymentTokens(const int count);

base::Value GetUnblindedPaymentTokensAsList(const int count);

}  // namespace privacy
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_UNBLINDED_PAYMENT_TOKENS_UNBLINDED_PAYMENT_TOKENS_UNITTEST_UTIL_H_
