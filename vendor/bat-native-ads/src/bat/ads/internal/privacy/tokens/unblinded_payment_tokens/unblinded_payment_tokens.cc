/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed w
 * h this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_tokens.h"

#include "base/check_op.h"
#include "base/containers/contains.h"
#include "base/ranges/algorithm.h"

namespace ads::privacy {

UnblindedPaymentTokens::UnblindedPaymentTokens() = default;

UnblindedPaymentTokens::~UnblindedPaymentTokens() = default;

const UnblindedPaymentTokenInfo& UnblindedPaymentTokens::GetToken() const {
  DCHECK_NE(Count(), 0);

  return unblinded_payment_tokens_.front();
}

const UnblindedPaymentTokenList& UnblindedPaymentTokens::GetAllTokens() const {
  return unblinded_payment_tokens_;
}

void UnblindedPaymentTokens::SetTokens(
    const UnblindedPaymentTokenList& unblinded_payment_tokens) {
  unblinded_payment_tokens_ = unblinded_payment_tokens;
}

void UnblindedPaymentTokens::AddTokens(
    const UnblindedPaymentTokenList& unblinded_payment_tokens) {
  for (const auto& unblinded_payment_token : unblinded_payment_tokens) {
    if (TokenExists(unblinded_payment_token)) {
      continue;
    }

    unblinded_payment_tokens_.push_back(unblinded_payment_token);
  }
}

bool UnblindedPaymentTokens::RemoveToken(
    const UnblindedPaymentTokenInfo& unblinded_payment_token) {
  auto iter =
      base::ranges::find(unblinded_payment_tokens_, unblinded_payment_token);

  if (iter == unblinded_payment_tokens_.cend()) {
    return false;
  }

  unblinded_payment_tokens_.erase(iter);

  return true;
}

void UnblindedPaymentTokens::RemoveTokens(
    const UnblindedPaymentTokenList& unblinded_payment_tokens) {
  const auto iter = std::remove_if(
      unblinded_payment_tokens_.begin(), unblinded_payment_tokens_.end(),
      [&unblinded_payment_tokens](
          const UnblindedPaymentTokenInfo& unblinded_payment_token) {
        return base::Contains(unblinded_payment_tokens,
                              unblinded_payment_token);
      });

  unblinded_payment_tokens_.erase(iter, unblinded_payment_tokens_.cend());
}

void UnblindedPaymentTokens::RemoveAllTokens() {
  unblinded_payment_tokens_.clear();
}

bool UnblindedPaymentTokens::TokenExists(
    const UnblindedPaymentTokenInfo& unblinded_payment_token) {
  return base::Contains(unblinded_payment_tokens_, unblinded_payment_token);
}

int UnblindedPaymentTokens::Count() const {
  return unblinded_payment_tokens_.size();
}

bool UnblindedPaymentTokens::IsEmpty() const {
  return unblinded_payment_tokens_.empty();
}

}  // namespace ads::privacy
