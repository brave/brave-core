/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PRIVACY_TOKENS_UNBLINDED_PAYMENT_TOKENS_UNBLINDED_PAYMENT_TOKENS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PRIVACY_TOKENS_UNBLINDED_PAYMENT_TOKENS_UNBLINDED_PAYMENT_TOKENS_H_

#include "brave/components/brave_ads/core/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_token_info.h"

namespace brave_ads::privacy {

class UnblindedPaymentTokens final {
 public:
  UnblindedPaymentTokens();

  UnblindedPaymentTokens(const UnblindedPaymentTokens&) = delete;
  UnblindedPaymentTokens& operator=(const UnblindedPaymentTokens&) = delete;

  UnblindedPaymentTokens(UnblindedPaymentTokens&&) noexcept = delete;
  UnblindedPaymentTokens& operator=(UnblindedPaymentTokens&&) noexcept = delete;

  ~UnblindedPaymentTokens();

  const UnblindedPaymentTokenInfo& GetToken() const;
  const UnblindedPaymentTokenList& GetAllTokens() const;

  void SetTokens(const UnblindedPaymentTokenList& unblinded_payment_tokens);

  void AddTokens(const UnblindedPaymentTokenList& unblinded_payment_tokens);

  bool RemoveToken(const UnblindedPaymentTokenInfo& unblinded_payment_token);
  void RemoveTokens(const UnblindedPaymentTokenList& unblinded_payment_tokens);
  void RemoveAllTokens();

  bool TokenExists(const UnblindedPaymentTokenInfo& unblinded_payment_token);

  size_t Count() const;

  bool IsEmpty() const;

 private:
  UnblindedPaymentTokenList unblinded_payment_tokens_;
};

}  // namespace brave_ads::privacy

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PRIVACY_TOKENS_UNBLINDED_PAYMENT_TOKENS_UNBLINDED_PAYMENT_TOKENS_H_
