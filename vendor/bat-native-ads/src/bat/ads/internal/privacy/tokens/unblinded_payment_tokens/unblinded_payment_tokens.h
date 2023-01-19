/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_TOKENS_UNBLINDED_PAYMENT_TOKENS_UNBLINDED_PAYMENT_TOKENS_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_TOKENS_UNBLINDED_PAYMENT_TOKENS_UNBLINDED_PAYMENT_TOKENS_H_

#include "bat/ads/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_token_info.h"

namespace ads::privacy {

class UnblindedPaymentTokens final {
 public:
  UnblindedPaymentTokens();

  UnblindedPaymentTokens(const UnblindedPaymentTokens& other) = delete;
  UnblindedPaymentTokens& operator=(const UnblindedPaymentTokens& other) =
      delete;

  UnblindedPaymentTokens(UnblindedPaymentTokens&& other) noexcept = delete;
  UnblindedPaymentTokens& operator=(UnblindedPaymentTokens&& other) noexcept =
      delete;

  ~UnblindedPaymentTokens();

  const UnblindedPaymentTokenInfo& GetToken() const;
  const UnblindedPaymentTokenList& GetAllTokens() const;

  void SetTokens(const UnblindedPaymentTokenList& unblinded_payment_tokens);

  void AddTokens(const UnblindedPaymentTokenList& unblinded_payment_tokens);

  bool RemoveToken(const UnblindedPaymentTokenInfo& unblinded_payment_token);
  void RemoveTokens(const UnblindedPaymentTokenList& unblinded_payment_tokens);
  void RemoveAllTokens();

  bool TokenExists(const UnblindedPaymentTokenInfo& unblinded_payment_token);

  int Count() const;

  bool IsEmpty() const;

 private:
  UnblindedPaymentTokenList unblinded_payment_tokens_;
};

}  // namespace ads::privacy

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_TOKENS_UNBLINDED_PAYMENT_TOKENS_UNBLINDED_PAYMENT_TOKENS_H_
