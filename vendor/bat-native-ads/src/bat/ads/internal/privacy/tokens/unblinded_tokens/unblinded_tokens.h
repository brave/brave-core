/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_TOKENS_UNBLINDED_TOKENS_UNBLINDED_TOKENS_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_TOKENS_UNBLINDED_TOKENS_UNBLINDED_TOKENS_H_

#include "bat/ads/internal/privacy/tokens/unblinded_tokens/unblinded_token_info.h"

namespace ads::privacy {

class UnblindedTokens final {
 public:
  UnblindedTokens();

  UnblindedTokens(const UnblindedTokens& other) = delete;
  UnblindedTokens& operator=(const UnblindedTokens& other) = delete;

  UnblindedTokens(UnblindedTokens&& other) noexcept = delete;
  UnblindedTokens& operator=(UnblindedTokens&& other) noexcept = delete;

  ~UnblindedTokens();

  const UnblindedTokenInfo& GetToken() const;
  const UnblindedTokenList& GetAllTokens() const;

  void SetTokens(const UnblindedTokenList& unblinded_tokens);

  void AddTokens(const UnblindedTokenList& unblinded_tokens);

  bool RemoveToken(const UnblindedTokenInfo& unblinded_token);
  void RemoveTokens(const UnblindedTokenList& unblinded_tokens);
  void RemoveAllTokens();

  bool TokenExists(const UnblindedTokenInfo& unblinded_token);

  int Count() const;

  bool IsEmpty() const;

 private:
  UnblindedTokenList unblinded_tokens_;
};

}  // namespace ads::privacy

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_TOKENS_UNBLINDED_TOKENS_UNBLINDED_TOKENS_H_
