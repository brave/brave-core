/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PRIVACY_TOKENS_UNBLINDED_TOKENS_UNBLINDED_TOKENS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PRIVACY_TOKENS_UNBLINDED_TOKENS_UNBLINDED_TOKENS_H_

#include "brave/components/brave_ads/core/internal/privacy/tokens/unblinded_tokens/unblinded_token_info.h"

namespace brave_ads::privacy {

class UnblindedTokens final {
 public:
  UnblindedTokens();

  UnblindedTokens(const UnblindedTokens&) = delete;
  UnblindedTokens& operator=(const UnblindedTokens&) = delete;

  UnblindedTokens(UnblindedTokens&&) noexcept = delete;
  UnblindedTokens& operator=(UnblindedTokens&&) noexcept = delete;

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

}  // namespace brave_ads::privacy

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PRIVACY_TOKENS_UNBLINDED_TOKENS_UNBLINDED_TOKENS_H_
