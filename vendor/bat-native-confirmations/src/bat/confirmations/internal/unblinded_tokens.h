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
  TokenList GetAllTokens() const;
  base::Value GetTokensAsList();

  void SetTokens(const TokenList& tokens);
  void SetTokensFromList(const base::Value& list);

  void AddTokens(const TokenList& tokens);

  bool RemoveToken(const TokenInfo& token);
  void RemoveAllTokens();

  bool TokenExists(const TokenInfo& token);

  int Count() const;

  bool IsEmpty() const;

 private:
  TokenList tokens_;

  ConfirmationsImpl* confirmations_;  // NOT OWNED
};

}  // namespace confirmations

#endif  // BAT_CONFIRMATIONS_INTERNAL_UNBLINDED_TOKENS_H_
