/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_tokens.h"

#include <algorithm>

namespace brave_ads {

PaymentTokens::PaymentTokens() = default;

PaymentTokens::~PaymentTokens() = default;

const PaymentTokenList& PaymentTokens::GetAllTokens() const {
  return payment_tokens_;
}

void PaymentTokens::SetTokens(const PaymentTokenList& payment_tokens) {
  payment_tokens_ = payment_tokens;
}

void PaymentTokens::AddTokens(const PaymentTokenList& payment_tokens) {
  payment_tokens_.reserve(payment_tokens_.size() + payment_tokens.size());

  for (const auto& payment_token : payment_tokens) {
    if (!TokenExists(payment_token)) {
      payment_tokens_.push_back(payment_token);
    }
  }
}

void PaymentTokens::RemoveTokens(const PaymentTokenList& payment_tokens) {
  std::erase_if(payment_tokens_,
                [&payment_tokens](const PaymentTokenInfo& payment_token) {
                  return std::ranges::contains(payment_tokens, payment_token);
                });
}

bool PaymentTokens::TokenExists(const PaymentTokenInfo& payment_token) const {
  return std::ranges::contains(payment_tokens_, payment_token);
}

size_t PaymentTokens::Count() const {
  return payment_tokens_.size();
}

bool PaymentTokens::IsEmpty() const {
  return payment_tokens_.empty();
}

}  // namespace brave_ads
