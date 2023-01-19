/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_TOKENS_UNBLINDED_TOKENS_UNBLINDED_TOKEN_VALUE_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_TOKENS_UNBLINDED_TOKENS_UNBLINDED_TOKEN_VALUE_UTIL_H_

#include "bat/ads/internal/privacy/tokens/unblinded_tokens/unblinded_token_info.h"

#include "base/values.h"

namespace ads::privacy {

base::Value::List UnblindedTokensToValue(
    const UnblindedTokenList& unblinded_tokens);

UnblindedTokenList UnblindedTokensFromValue(const base::Value::List& list);

}  // namespace ads::privacy

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_TOKENS_UNBLINDED_TOKENS_UNBLINDED_TOKEN_VALUE_UTIL_H_
