/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens_util.h"

#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens.h"
#include "brave/components/brave_ads/core/internal/deprecated/confirmations/confirmation_state_manager.h"

namespace brave_ads {

namespace {

bool HasConfirmationTokens() {
  return ConfirmationTokenCount() > 0;
}

}  // namespace

absl::optional<ConfirmationTokenInfo> MaybeGetConfirmationToken() {
  if (!HasConfirmationTokens()) {
    return absl::nullopt;
  }

  return ConfirmationStateManager::GetInstance()
      .GetConfirmationTokens()
      .GetToken();
}

void AddConfirmationTokens(const ConfirmationTokenList& confirmation_tokens) {
  ConfirmationStateManager::GetInstance().GetConfirmationTokens().AddTokens(
      confirmation_tokens);

  ConfirmationStateManager::GetInstance().SaveState();
}

bool RemoveConfirmationToken(const ConfirmationTokenInfo& confirmation_token) {
  if (!ConfirmationStateManager::GetInstance()
           .GetConfirmationTokens()
           .RemoveToken(confirmation_token)) {
    return false;
  }

  ConfirmationStateManager::GetInstance().SaveState();

  return true;
}

void RemoveConfirmationTokens(
    const ConfirmationTokenList& confirmation_tokens) {
  ConfirmationStateManager::GetInstance().GetConfirmationTokens().RemoveTokens(
      confirmation_tokens);

  ConfirmationStateManager::GetInstance().SaveState();
}

void RemoveAllConfirmationTokens() {
  ConfirmationStateManager::GetInstance()
      .GetConfirmationTokens()
      .RemoveAllTokens();

  ConfirmationStateManager::GetInstance().SaveState();
}

bool ConfirmationTokenExists(const ConfirmationTokenInfo& confirmation_token) {
  return ConfirmationStateManager::GetInstance()
      .GetConfirmationTokens()
      .TokenExists(confirmation_token);
}

bool ConfirmationTokensIsEmpty() {
  return ConfirmationStateManager::GetInstance()
      .GetConfirmationTokens()
      .IsEmpty();
}

int ConfirmationTokenCount() {
  return ConfirmationStateManager::GetInstance()
      .GetConfirmationTokens()
      .Count();
}

bool IsValid(const ConfirmationTokenInfo& confirmation_token) {
  return confirmation_token.unblinded_token.has_value() &&
         confirmation_token.public_key.has_value() &&
         !confirmation_token.signature.empty();
}

}  // namespace brave_ads
