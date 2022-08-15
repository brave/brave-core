/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_TOKENS_UNBLINDED_TOKENS_UNBLINDED_TOKENS_UNITTEST_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_TOKENS_UNBLINDED_TOKENS_UNBLINDED_TOKENS_UNITTEST_UTIL_H_

#include <string>
#include <vector>

#include "bat/ads/internal/privacy/tokens/unblinded_tokens/unblinded_token_info.h"

namespace base {
class Value;
}  // namespace base

namespace ads {
namespace privacy {

class UnblindedTokens;

UnblindedTokens* GetUnblindedTokens();

UnblindedTokenList SetUnblindedTokens(const int count);

UnblindedTokenInfo CreateUnblindedToken(
    const std::string& unblinded_token_base64);

UnblindedTokenList CreateUnblindedTokens(
    const std::vector<std::string>& unblinded_tokens_base64);

UnblindedTokenList GetUnblindedTokens(const int count);

UnblindedTokenList GetRandomUnblindedTokens(const int count);

base::Value GetUnblindedTokensAsList(const int count);

}  // namespace privacy
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_TOKENS_UNBLINDED_TOKENS_UNBLINDED_TOKENS_UNITTEST_UTIL_H_
