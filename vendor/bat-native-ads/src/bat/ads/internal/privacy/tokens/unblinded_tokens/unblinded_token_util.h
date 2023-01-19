/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_TOKENS_UNBLINDED_TOKENS_UNBLINDED_TOKEN_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_TOKENS_UNBLINDED_TOKENS_UNBLINDED_TOKEN_UTIL_H_

#include "absl/types/optional.h"
#include "bat/ads/internal/privacy/tokens/unblinded_tokens/unblinded_token_info.h"

namespace ads::privacy {

absl::optional<UnblindedTokenInfo> MaybeGetUnblindedToken();

const UnblindedTokenList& GetAllUnblindedTokens();

void AddUnblindedTokens(const UnblindedTokenList& unblinded_tokens);

bool RemoveUnblindedToken(const UnblindedTokenInfo& unblinded_token);
void RemoveUnblindedTokens(const UnblindedTokenList& unblinded_tokens);
void RemoveAllUnblindedTokens();

bool UnblindedTokenExists(const UnblindedTokenInfo& unblinded_token);

bool UnblindedTokensIsEmpty();

int UnblindedTokenCount();

bool IsValid(const UnblindedTokenInfo& unblinded_token);

}  // namespace ads::privacy

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_TOKENS_UNBLINDED_TOKENS_UNBLINDED_TOKEN_UTIL_H_
