/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_CONFIRMATIONS_UNBLINDED_TOKENS_H_
#define BAT_CONFIRMATIONS_UNBLINDED_TOKENS_H_

#include <string>
#include <vector>

#include "base/values.h"

#include "wrapper.hpp"

using challenge_bypass_ristretto::UnblindedToken;

namespace confirmations {

class ConfirmationsImpl;

class UnblindedTokens {
 public:
  explicit UnblindedTokens(ConfirmationsImpl* confirmations);
  ~UnblindedTokens();

  UnblindedToken GetToken() const;
  std::vector<UnblindedToken> GetAllTokens() const;
  base::Value GetTokensAsList();

  void SetTokens(const std::vector<UnblindedToken>& tokens);
  void SetTokensFromList(const base::Value& list);

  void AddTokens(const std::vector<UnblindedToken>& tokens);

  bool RemoveToken(const UnblindedToken& token);
  void RemoveAllTokens();

  bool TokenExists(const UnblindedToken& token);

  int Count() const;

  bool IsEmpty() const;

 private:
  std::vector<UnblindedToken> unblinded_tokens_;

  ConfirmationsImpl* confirmations_;  // NOT OWNED
};

}  // namespace confirmations

#endif  // BAT_CONFIRMATIONS_UNBLINDED_TOKENS_H_
