/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/privacy/tokens/unblinded_tokens/unblinded_token_util.h"

#include "bat/ads/internal/deprecated/confirmations/confirmation_state_manager.h"
#include "bat/ads/internal/privacy/tokens/unblinded_tokens/unblinded_tokens.h"

namespace ads::privacy {

namespace {

bool HasUnblindedTokens() {
  return UnblindedTokenCount() > 0;
}

}  // namespace

absl::optional<UnblindedTokenInfo> MaybeGetUnblindedToken() {
  if (!HasUnblindedTokens()) {
    return absl::nullopt;
  }

  return ConfirmationStateManager::GetInstance()
      ->GetUnblindedTokens()
      ->GetToken();
}

const UnblindedTokenList& GetAllUnblindedTokens() {
  return ConfirmationStateManager::GetInstance()
      ->GetUnblindedTokens()
      ->GetAllTokens();
}

void AddUnblindedTokens(const UnblindedTokenList& unblinded_tokens) {
  ConfirmationStateManager::GetInstance()->GetUnblindedTokens()->AddTokens(
      unblinded_tokens);

  ConfirmationStateManager::GetInstance()->Save();
}

bool RemoveUnblindedToken(const UnblindedTokenInfo& unblinded_token) {
  if (!ConfirmationStateManager::GetInstance()
           ->GetUnblindedTokens()
           ->RemoveToken(unblinded_token)) {
    return false;
  }

  ConfirmationStateManager::GetInstance()->Save();

  return true;
}

void RemoveUnblindedTokens(const UnblindedTokenList& unblinded_tokens) {
  ConfirmationStateManager::GetInstance()->GetUnblindedTokens()->RemoveTokens(
      unblinded_tokens);

  ConfirmationStateManager::GetInstance()->Save();
}

void RemoveAllUnblindedTokens() {
  ConfirmationStateManager::GetInstance()
      ->GetUnblindedTokens()
      ->RemoveAllTokens();

  ConfirmationStateManager::GetInstance()->Save();
}

bool UnblindedTokenExists(const UnblindedTokenInfo& unblinded_token) {
  return ConfirmationStateManager::GetInstance()
      ->GetUnblindedTokens()
      ->TokenExists(unblinded_token);
}

bool UnblindedTokensIsEmpty() {
  return ConfirmationStateManager::GetInstance()
      ->GetUnblindedTokens()
      ->IsEmpty();
}

int UnblindedTokenCount() {
  return ConfirmationStateManager::GetInstance()->GetUnblindedTokens()->Count();
}

bool IsValid(const UnblindedTokenInfo& unblinded_token) {
  return unblinded_token.value.has_value() &&
         unblinded_token.public_key.has_value() &&
         !unblinded_token.signature.empty();
}

}  // namespace ads::privacy
