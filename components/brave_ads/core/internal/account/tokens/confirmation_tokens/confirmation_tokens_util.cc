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

ConfirmationTokens& GetConfirmationTokens() {
  return ConfirmationStateManager::GetInstance().GetConfirmationTokens();
}

std::optional<ConfirmationTokenInfo> MaybeGetConfirmationToken() {
  if (!HasConfirmationTokens()) {
    return std::nullopt;
  }

  return GetConfirmationTokens().Get();
}

void AddConfirmationTokens(const ConfirmationTokenList& confirmation_tokens) {
  GetConfirmationTokens().Add(confirmation_tokens);

  ConfirmationStateManager::GetInstance().SaveState();
}

bool RemoveConfirmationToken(const ConfirmationTokenInfo& confirmation_token) {
  if (!GetConfirmationTokens().Remove(confirmation_token)) {
    return false;
  }

  ConfirmationStateManager::GetInstance().SaveState();

  return true;
}

void RemoveConfirmationTokens(
    const ConfirmationTokenList& confirmation_tokens) {
  GetConfirmationTokens().Remove(confirmation_tokens);

  ConfirmationStateManager::GetInstance().SaveState();
}

void RemoveAllConfirmationTokens() {
  GetConfirmationTokens().RemoveAll();

  ConfirmationStateManager::GetInstance().SaveState();
}

bool ConfirmationTokenExists(const ConfirmationTokenInfo& confirmation_token) {
  return GetConfirmationTokens().Exists(confirmation_token);
}

bool ConfirmationTokensIsEmpty() {
  return GetConfirmationTokens().IsEmpty();
}

size_t ConfirmationTokenCount() {
  return GetConfirmationTokens().Count();
}

bool IsValid(const ConfirmationTokenInfo& confirmation_token) {
  return confirmation_token.unblinded_token.has_value() &&
         confirmation_token.public_key.has_value() &&
         !confirmation_token.signature_base64.empty();
}

}  // namespace brave_ads
