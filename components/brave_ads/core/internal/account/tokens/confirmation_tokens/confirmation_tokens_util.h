/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_TOKENS_CONFIRMATION_TOKENS_CONFIRMATION_TOKENS_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_TOKENS_CONFIRMATION_TOKENS_CONFIRMATION_TOKENS_UTIL_H_

#include <cstddef>
#include <optional>

#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_token_info.h"

namespace brave_ads {

class ConfirmationTokens;

ConfirmationTokens& GetConfirmationTokens();

std::optional<ConfirmationTokenInfo> MaybeGetConfirmationToken();

void AddConfirmationTokens(const ConfirmationTokenList& confirmation_tokens);

bool RemoveConfirmationToken(const ConfirmationTokenInfo& confirmation_token);
void RemoveConfirmationTokens(const ConfirmationTokenList& confirmation_tokens);
void RemoveAllConfirmationTokens();

bool ConfirmationTokenExists(const ConfirmationTokenInfo& confirmation_token);

bool ConfirmationTokensIsEmpty();

size_t ConfirmationTokenCount();

[[nodiscard]] bool IsValid(const ConfirmationTokenInfo& confirmation_token);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_TOKENS_CONFIRMATION_TOKENS_CONFIRMATION_TOKENS_UTIL_H_
