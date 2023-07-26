/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PRIVACY_TOKENS_CONFIRMATION_TOKENS_CONFIRMATION_TOKENS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PRIVACY_TOKENS_CONFIRMATION_TOKENS_CONFIRMATION_TOKENS_H_

#include "brave/components/brave_ads/core/internal/privacy/tokens/confirmation_tokens/confirmation_token_info.h"

namespace brave_ads::privacy {

class ConfirmationTokens final {
 public:
  ConfirmationTokens();

  ConfirmationTokens(const ConfirmationTokens&) = delete;
  ConfirmationTokens& operator=(const ConfirmationTokens&) = delete;

  ConfirmationTokens(ConfirmationTokens&&) noexcept = delete;
  ConfirmationTokens& operator=(ConfirmationTokens&&) noexcept = delete;

  ~ConfirmationTokens();

  const ConfirmationTokenInfo& GetToken() const;
  const ConfirmationTokenList& GetAllTokens() const;

  void SetTokens(const ConfirmationTokenList& confirmation_tokens);

  void AddTokens(const ConfirmationTokenList& confirmation_tokens);

  bool RemoveToken(const ConfirmationTokenInfo& confirmation_token);
  void RemoveTokens(const ConfirmationTokenList& confirmation_tokens);
  void RemoveAllTokens();

  bool TokenExists(const ConfirmationTokenInfo& confirmation_token) const;

  int Count() const;

  bool IsEmpty() const;

 private:
  ConfirmationTokenList confirmation_tokens_;
};

}  // namespace brave_ads::privacy

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PRIVACY_TOKENS_CONFIRMATION_TOKENS_CONFIRMATION_TOKENS_H_
