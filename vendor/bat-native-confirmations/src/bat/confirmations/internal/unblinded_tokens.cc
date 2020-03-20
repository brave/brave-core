/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>
#include <algorithm>

#include "bat/confirmations/internal/unblinded_tokens.h"
#include "bat/confirmations/internal/confirmations_impl.h"

#include "base/logging.h"

namespace confirmations {

UnblindedTokens::UnblindedTokens(ConfirmationsImpl* confirmations) :
    confirmations_(confirmations) {
}

UnblindedTokens::~UnblindedTokens() = default;

TokenInfo UnblindedTokens::GetToken() const {
  DCHECK_NE(Count(), 0);
  return tokens_.front();
}

TokenList UnblindedTokens::GetAllTokens() const {
  return tokens_;
}

base::Value UnblindedTokens::GetTokensAsList() {
  base::Value list(base::Value::Type::LIST);
  for (const auto& token : tokens_) {
    base::Value dictionary(base::Value::Type::DICTIONARY);
    dictionary.SetKey("unblinded_token", base::Value(
        token.unblinded_token.encode_base64()));
    dictionary.SetKey("public_key", base::Value(token.public_key));

    list.Append(std::move(dictionary));
  }

  return list;
}

void UnblindedTokens::SetTokens(
    const TokenList& tokens) {
  tokens_ = tokens;

  confirmations_->SaveState();
}

void UnblindedTokens::SetTokensFromList(const base::Value& list) {
  base::ListValue list_values(list.GetList());

  TokenList tokens;
  for (auto& value : list_values) {
    std::string unblinded_token;
    std::string public_key;

    if (value.is_string()) {
      // Migrate legacy tokens
      unblinded_token = value.GetString();
      public_key = "";
    } else {
      base::DictionaryValue* dictionary;
      if (!value.GetAsDictionary(&dictionary)) {
        DCHECK(false) << "Unblinded token should be a dictionary";
        continue;
      }

      // Unblinded token
      auto* unblinded_token_value = dictionary->FindKey("unblinded_token");
      if (!unblinded_token_value) {
        DCHECK(false) << "Unblinded token dictionary missing unblinded_token";
        continue;
      }
      unblinded_token = unblinded_token_value->GetString();

      // Public key
      auto* public_key_value = dictionary->FindKey("public_key");
      if (!public_key_value) {
        DCHECK(false) << "Unblinded token dictionary missing public_key";
        continue;
      }
      public_key = public_key_value->GetString();
    }

    TokenInfo token_info;
    token_info.unblinded_token = UnblindedToken::decode_base64(unblinded_token);
    token_info.public_key = public_key;

    tokens.push_back(token_info);
  }

  SetTokens(tokens);
}

void UnblindedTokens::AddTokens(
    const TokenList& tokens) {
  for (const auto& token_info : tokens) {
    if (TokenExists(token_info)) {
      continue;
    }

    tokens_.push_back(token_info);
  }

  confirmations_->SaveState();
}

bool UnblindedTokens::RemoveToken(const TokenInfo& token) {
  auto unblinded_token = token.unblinded_token;
  auto public_key = token.public_key;

  auto it = std::find_if(tokens_.begin(), tokens_.end(),
      [=](const TokenInfo& info) {
        return (info.unblinded_token == unblinded_token);
      });

  if (it == tokens_.end()) {
    return false;
  }

  tokens_.erase(it);

  confirmations_->SaveState();

  return true;
}

void UnblindedTokens::RemoveAllTokens() {
  tokens_.clear();

  confirmations_->SaveState();
}

bool UnblindedTokens::TokenExists(const TokenInfo& token) {
  auto unblinded_token = token.unblinded_token;
  auto public_key = token.public_key;

  auto it = std::find_if(tokens_.begin(), tokens_.end(),
      [=](const TokenInfo& info) {
        return (info.unblinded_token == unblinded_token);
      });

  if (it == tokens_.end()) {
    return false;
  }

  return true;
}

int UnblindedTokens::Count() const {
  return tokens_.size();
}

bool UnblindedTokens::IsEmpty() const {
  if (Count() > 0) {
    return false;
  }

  return true;
}

}  // namespace confirmations
