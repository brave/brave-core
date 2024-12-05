/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_TOKENS_CONFIRMATION_TOKENS_CONFIRMATION_TOKENS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_TOKENS_CONFIRMATION_TOKENS_CONFIRMATION_TOKENS_H_

#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_token_info.h"

#include <cstddef>

namespace brave_ads {

class ConfirmationTokens final {
 public:
  ConfirmationTokens();

  ConfirmationTokens(const ConfirmationTokens&) = delete;
  ConfirmationTokens& operator=(const ConfirmationTokens&) = delete;

  ~ConfirmationTokens();

  const ConfirmationTokenInfo& Get() const;
  const ConfirmationTokenList& GetAll() const;

  void Set(const ConfirmationTokenList& confirmation_tokens);

  void Add(const ConfirmationTokenList& confirmation_tokens);

  bool Remove(const ConfirmationTokenInfo& confirmation_token);
  void Remove(const ConfirmationTokenList& confirmation_tokens);
  void RemoveAll();

  bool Exists(const ConfirmationTokenInfo& confirmation_token) const;

  size_t Count() const;

  bool IsEmpty() const;

 private:
  ConfirmationTokenList confirmation_tokens_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_TOKENS_CONFIRMATION_TOKENS_CONFIRMATION_TOKENS_H_
