/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens.h"

#include <vector>

#include "base/check_op.h"
#include "base/containers/contains.h"
#include "base/ranges/algorithm.h"

namespace brave_ads {

ConfirmationTokens::ConfirmationTokens() = default;

ConfirmationTokens::~ConfirmationTokens() = default;

const ConfirmationTokenInfo& ConfirmationTokens::GetToken() const {
  CHECK_NE(0U, Count());

  return confirmation_tokens_.front();
}

const ConfirmationTokenList& ConfirmationTokens::GetAllTokens() const {
  return confirmation_tokens_;
}

void ConfirmationTokens::SetTokens(
    const ConfirmationTokenList& confirmation_tokens) {
  confirmation_tokens_ = confirmation_tokens;
}

void ConfirmationTokens::AddTokens(
    const ConfirmationTokenList& confirmation_tokens) {
  confirmation_tokens_.reserve(confirmation_tokens_.size() +
                               confirmation_tokens.size());

  for (const auto& confirmation_token : confirmation_tokens) {
    if (!TokenExists(confirmation_token)) {
      confirmation_tokens_.push_back(confirmation_token);
    }
  }
}

bool ConfirmationTokens::RemoveToken(
    const ConfirmationTokenInfo& confirmation_token) {
  const auto iter =
      base::ranges::find(confirmation_tokens_, confirmation_token);
  if (iter == confirmation_tokens_.cend()) {
    return false;
  }

  confirmation_tokens_.erase(iter);

  return true;
}

void ConfirmationTokens::RemoveTokens(
    const ConfirmationTokenList& confirmation_tokens) {
  std::erase_if(
      confirmation_tokens_,
      [&confirmation_tokens](const ConfirmationTokenInfo& confirmation_token) {
        return base::Contains(confirmation_tokens, confirmation_token);
      });
}

void ConfirmationTokens::RemoveAllTokens() {
  confirmation_tokens_.clear();
}

bool ConfirmationTokens::TokenExists(
    const ConfirmationTokenInfo& confirmation_token) const {
  return base::Contains(confirmation_tokens_, confirmation_token);
}

size_t ConfirmationTokens::Count() const {
  return confirmation_tokens_.size();
}

bool ConfirmationTokens::IsEmpty() const {
  return confirmation_tokens_.empty();
}

}  // namespace brave_ads
