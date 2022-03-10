/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed w
 * h this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/privacy/unblinded_tokens/unblinded_tokens.h"

#include <string>
#include <utility>

#include "base/check_op.h"
#include "base/values.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/privacy/unblinded_tokens/unblinded_token_info.h"

namespace ads {
namespace privacy {

UnblindedTokens::UnblindedTokens() = default;

UnblindedTokens::~UnblindedTokens() = default;

UnblindedTokenInfo UnblindedTokens::GetToken() const {
  DCHECK_NE(Count(), 0);

  return unblinded_tokens_.front();
}

UnblindedTokenList UnblindedTokens::GetAllTokens() const {
  return unblinded_tokens_;
}

base::Value UnblindedTokens::GetTokensAsList() {
  base::Value list(base::Value::Type::LIST);

  for (const auto& unblinded_token : unblinded_tokens_) {
    base::Value dictionary(base::Value::Type::DICTIONARY);
    dictionary.SetKey("unblinded_token",
                      base::Value(unblinded_token.value.encode_base64()));
    dictionary.SetKey("public_key",
                      base::Value(unblinded_token.public_key.encode_base64()));

    list.Append(std::move(dictionary));
  }

  return list;
}

void UnblindedTokens::SetTokens(const UnblindedTokenList& unblinded_tokens) {
  unblinded_tokens_ = unblinded_tokens;
}

void UnblindedTokens::SetTokensFromList(const base::Value& list) {
  UnblindedTokenList unblinded_tokens;

  for (const auto& value : list.GetList()) {
    std::string unblinded_token_base64;
    std::string public_key_base64;

    if (value.is_string()) {
      // Migrate legacy tokens
      unblinded_token_base64 = value.GetString();
    } else {
      const base::DictionaryValue* dictionary = nullptr;
      if (!value.GetAsDictionary(&dictionary)) {
        BLOG(0, "Unblinded token should be a dictionary");
        continue;
      }

      // Unblinded token
      const std::string* unblinded_token =
          dictionary->FindStringKey("unblinded_token");
      if (!unblinded_token) {
        BLOG(0, "Unblinded token dictionary missing unblinded_token");
        continue;
      }
      unblinded_token_base64 = *unblinded_token;

      // Public key
      const std::string* public_key = dictionary->FindStringKey("public_key");
      if (!public_key) {
        BLOG(0, "Unblinded token dictionary missing public_key");
        continue;
      }
      public_key_base64 = *public_key;
    }

    UnblindedTokenInfo unblinded_token;
    unblinded_token.value =
        UnblindedToken::decode_base64(unblinded_token_base64);
    unblinded_token.public_key = PublicKey::decode_base64(public_key_base64);

    unblinded_tokens.push_back(unblinded_token);
  }

  SetTokens(unblinded_tokens);
}

void UnblindedTokens::AddTokens(const UnblindedTokenList& unblinded_tokens) {
  for (const auto& unblinded_token : unblinded_tokens) {
    if (TokenExists(unblinded_token)) {
      continue;
    }

    unblinded_tokens_.push_back(unblinded_token);
  }
}

bool UnblindedTokens::RemoveToken(const UnblindedTokenInfo& unblinded_token) {
  auto iter = std::find_if(unblinded_tokens_.cbegin(), unblinded_tokens_.cend(),
                           [&unblinded_token](const UnblindedTokenInfo& value) {
                             return unblinded_token == value;
                           });

  if (iter == unblinded_tokens_.end()) {
    return false;
  }

  unblinded_tokens_.erase(iter);

  return true;
}

void UnblindedTokens::RemoveTokens(const UnblindedTokenList& unblinded_tokens) {
  const auto iter = std::remove_if(
      unblinded_tokens_.begin(), unblinded_tokens_.end(),
      [&unblinded_tokens](const UnblindedTokenInfo& unblinded_token) {
        return std::find(unblinded_tokens.cbegin(), unblinded_tokens.cend(),
                         unblinded_token) != unblinded_tokens.end();
      });

  unblinded_tokens_.erase(iter, unblinded_tokens_.end());
}

void UnblindedTokens::RemoveAllTokens() {
  unblinded_tokens_.clear();
}

bool UnblindedTokens::TokenExists(const UnblindedTokenInfo& unblinded_token) {
  auto iter = std::find_if(unblinded_tokens_.cbegin(), unblinded_tokens_.cend(),
                           [&unblinded_token](const UnblindedTokenInfo& value) {
                             return unblinded_token == value;
                           });

  if (iter == unblinded_tokens_.end()) {
    return false;
  }

  return true;
}

int UnblindedTokens::Count() const {
  return unblinded_tokens_.size();
}

bool UnblindedTokens::IsEmpty() const {
  return unblinded_tokens_.empty();
}

}  // namespace privacy
}  // namespace ads
