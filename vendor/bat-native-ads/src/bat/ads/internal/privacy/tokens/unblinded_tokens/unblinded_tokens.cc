/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed w
 * h this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/privacy/tokens/unblinded_tokens/unblinded_tokens.h"

#include "base/check_op.h"
#include "base/containers/contains.h"

namespace ads::privacy {

UnblindedTokens::UnblindedTokens() = default;

UnblindedTokens::~UnblindedTokens() = default;

const UnblindedTokenInfo& UnblindedTokens::GetToken() const {
  DCHECK_NE(Count(), 0);

  return unblinded_tokens_.front();
}

const UnblindedTokenList& UnblindedTokens::GetAllTokens() const {
  return unblinded_tokens_;
}

void UnblindedTokens::SetTokens(const UnblindedTokenList& unblinded_tokens) {
  unblinded_tokens_ = unblinded_tokens;
}

void UnblindedTokens::AddTokens(const UnblindedTokenList& unblinded_tokens) {
  for (const auto& unblinded_token : unblinded_tokens) {
    if (TokenExists(unblinded_token)) {
      continue;
    }

    unblinded_tokens_.push_back(unblinded_token);
  }
}

bool UnblindedTokens::RemoveToken(const UnblindedTokenInfo& unblinded_token) {
  auto iter = base::ranges::find(unblinded_tokens_, unblinded_token);

  if (iter == unblinded_tokens_.cend()) {
    return false;
  }

  unblinded_tokens_.erase(iter);

  return true;
}

void UnblindedTokens::RemoveTokens(const UnblindedTokenList& unblinded_tokens) {
  const auto iter = std::remove_if(
      unblinded_tokens_.begin(), unblinded_tokens_.end(),
      [&unblinded_tokens](const UnblindedTokenInfo& unblinded_token) {
        return base::Contains(unblinded_tokens, unblinded_token);
      });

  unblinded_tokens_.erase(iter, unblinded_tokens_.cend());
}

void UnblindedTokens::RemoveAllTokens() {
  unblinded_tokens_.clear();
}

bool UnblindedTokens::TokenExists(const UnblindedTokenInfo& unblinded_token) {
  return base::Contains(unblinded_tokens_, unblinded_token);
}

int UnblindedTokens::Count() const {
  return unblinded_tokens_.size();
}

bool UnblindedTokens::IsEmpty() const {
  return unblinded_tokens_.empty();
}

}  // namespace ads::privacy
