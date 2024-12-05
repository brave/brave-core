/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_TOKENS_PAYMENT_TOKENS_PAYMENT_TOKENS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_TOKENS_PAYMENT_TOKENS_PAYMENT_TOKENS_H_

#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_token_info.h"

#include <cstddef>

namespace brave_ads {

class PaymentTokens final {
 public:
  PaymentTokens();

  PaymentTokens(const PaymentTokens&) = delete;
  PaymentTokens& operator=(const PaymentTokens&) = delete;

  ~PaymentTokens();

  const PaymentTokenInfo& GetToken() const;
  const PaymentTokenList& GetAllTokens() const;

  void SetTokens(const PaymentTokenList& payment_tokens);

  void AddTokens(const PaymentTokenList& payment_tokens);

  bool RemoveToken(const PaymentTokenInfo& payment_token);
  void RemoveTokens(const PaymentTokenList& payment_tokens);
  void RemoveAllTokens();

  bool TokenExists(const PaymentTokenInfo& payment_token);

  size_t Count() const;

  bool IsEmpty() const;

 private:
  PaymentTokenList payment_tokens_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_TOKENS_PAYMENT_TOKENS_PAYMENT_TOKENS_H_
