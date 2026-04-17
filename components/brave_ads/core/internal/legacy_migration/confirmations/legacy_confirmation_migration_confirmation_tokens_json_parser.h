/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_LEGACY_MIGRATION_CONFIRMATIONS_LEGACY_CONFIRMATION_MIGRATION_CONFIRMATION_TOKENS_JSON_PARSER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_LEGACY_MIGRATION_CONFIRMATIONS_LEGACY_CONFIRMATION_MIGRATION_CONFIRMATION_TOKENS_JSON_PARSER_H_

#include <optional>
#include <string_view>

#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_token_info.h"

namespace brave_ads {

struct WalletInfo;

namespace json::reader {

// Parses confirmation tokens from the legacy `confirmations.json` format.
// Tokens whose Ed25519 signature cannot be verified against the wallet public
// key are silently dropped. Returns `std::nullopt` if `json` is malformed or
// the `unblinded_tokens` key is absent.
std::optional<ConfirmationTokenList> ParseConfirmationTokens(
    std::string_view json,
    const WalletInfo& wallet);

}  // namespace json::reader

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_LEGACY_MIGRATION_CONFIRMATIONS_LEGACY_CONFIRMATION_MIGRATION_CONFIRMATION_TOKENS_JSON_PARSER_H_
