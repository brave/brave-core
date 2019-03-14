/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_CONFIRMATIONS_INTERNAL_UNBLINDED_TOKENS_H_
#define BAT_CONFIRMATIONS_INTERNAL_UNBLINDED_TOKENS_H_

#include <string>
#include <vector>

#include "bat/confirmations/internal/token_info.h"

#include "base/values.h"

namespace confirmations {

class ConfirmationsImpl;

class UnblindedTokens {
 public:
  explicit UnblindedTokens(ConfirmationsImpl* confirmations);
  ~UnblindedTokens();

  TokenInfo GetToken() const;
  std::vector<TokenInfo> GetAllTokens() const;
  base::Value GetTokensAsList();

  void SetTokens(const std::vector<TokenInfo>& tokens);
  void SetTokensFromList(const base::Value& list);

  void AddTokens(const std::vector<TokenInfo>& tokens);

  bool RemoveToken(const TokenInfo& unblinded_token);
  void RemoveAllTokens();

  bool TokenExists(const TokenInfo& unblinded_token);

  int Count() const;

  bool IsEmpty() const;

 private:
  std::vector<TokenInfo> tokens_;

  ConfirmationsImpl* confirmations_;  // NOT OWNED
};

}  // namespace confirmations

#endif  // BAT_CONFIRMATIONS_INTERNAL_UNBLINDED_TOKENS_H_
