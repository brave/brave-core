/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens_util.h"

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_token_info.h"
#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens.h"
#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens_database_table.h"
#include "brave/components/brave_ads/core/internal/account/tokens/token_state_manager.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"

namespace brave_ads {

namespace {

bool HasConfirmationTokens() {
  return ConfirmationTokenCount() > 0;
}

}  // namespace

ConfirmationTokens& GetConfirmationTokens() {
  return TokenStateManager::GetInstance().GetConfirmationTokens();
}

std::optional<ConfirmationTokenInfo> MaybeGetConfirmationToken() {
  if (!HasConfirmationTokens()) {
    return std::nullopt;
  }

  return GetConfirmationTokens().Get();
}

void AddConfirmationTokens(const ConfirmationTokenList& confirmation_tokens) {
  GetConfirmationTokens().Add(confirmation_tokens);

  database::table::ConfirmationTokens confirmation_tokens_database_table;
  confirmation_tokens_database_table.Save(
      confirmation_tokens, base::BindOnce([](bool success) {
        if (!success) {
          BLOG(0, "Failed to save confirmation tokens");
        }
      }));
}

bool RemoveConfirmationToken(const ConfirmationTokenInfo& confirmation_token) {
  if (!GetConfirmationTokens().Remove(confirmation_token)) {
    return false;
  }

  database::table::ConfirmationTokens confirmation_tokens_database_table;
  confirmation_tokens_database_table.Delete(
      confirmation_token, base::BindOnce([](bool success) {
        if (!success) {
          BLOG(0, "Failed to delete confirmation token");
        }
      }));

  return true;
}

size_t ConfirmationTokenCount() {
  return GetConfirmationTokens().Count();
}

}  // namespace brave_ads
