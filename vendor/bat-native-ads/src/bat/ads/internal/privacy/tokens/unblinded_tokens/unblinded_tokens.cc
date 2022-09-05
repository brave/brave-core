/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed w
 * h this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/privacy/tokens/unblinded_tokens/unblinded_tokens.h"

#include <algorithm>
#include <string>
#include <utility>

#include "absl/types/optional.h"
#include "base/check_op.h"
#include "base/notreached.h"
#include "bat/ads/internal/base/logging_util.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/unblinded_token.h"

namespace ads {
namespace privacy {

UnblindedTokens::UnblindedTokens() = default;

UnblindedTokens::~UnblindedTokens() = default;

const UnblindedTokenInfo& UnblindedTokens::GetToken() const {
  DCHECK_NE(Count(), 0);

  return unblinded_tokens_.front();
}

const UnblindedTokenList& UnblindedTokens::GetAllTokens() const {
  return unblinded_tokens_;
}

base::Value::List UnblindedTokens::GetTokensAsList() {
  base::Value::List list;

  for (const auto& unblinded_token : unblinded_tokens_) {
    const absl::optional<std::string> unblinded_token_base64 =
        unblinded_token.value.EncodeBase64();
    if (!unblinded_token_base64) {
      NOTREACHED();
      continue;
    }

    const absl::optional<std::string> public_key_base64 =
        unblinded_token.public_key.EncodeBase64();
    if (!public_key_base64) {
      NOTREACHED();
      continue;
    }
    base::Value::Dict dict;
    dict.Set("unblinded_token", *unblinded_token_base64);
    dict.Set("public_key", *public_key_base64);
    list.Append(std::move(dict));
  }

  return list;
}

void UnblindedTokens::SetTokens(const UnblindedTokenList& unblinded_tokens) {
  unblinded_tokens_ = unblinded_tokens;
}

void UnblindedTokens::SetTokensFromList(const base::Value::List& list) {
  UnblindedTokenList unblinded_tokens;

  for (const auto& item : list) {
    std::string unblinded_token_base64;
    std::string public_key_base64;

    if (item.is_string()) {
      // Migrate legacy tokens
      unblinded_token_base64 = item.GetString();
    } else {
      if (!item.is_dict()) {
        BLOG(0, "Unblinded token should be a dictionary");
        continue;
      }
      const base::Value::Dict& dict = item.GetDict();

      // Unblinded token
      const std::string* unblinded_token = dict.FindString("unblinded_token");
      if (!unblinded_token) {
        BLOG(0, "Unblinded token dictionary missing unblinded_token");
        continue;
      }
      unblinded_token_base64 = *unblinded_token;

      // Public key
      const std::string* public_key = dict.FindString("public_key");
      if (!public_key) {
        BLOG(0, "Unblinded token dictionary missing public_key");
        continue;
      }
      public_key_base64 = *public_key;
    }

    UnblindedTokenInfo unblinded_token;
    unblinded_token.value = cbr::UnblindedToken(unblinded_token_base64);
    unblinded_token.public_key = cbr::PublicKey(public_key_base64);
    if (!unblinded_token.is_valid()) {
      BLOG(0, "Unblinded token is invalid");
      continue;
    }

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

  if (iter == unblinded_tokens_.cend()) {
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
                         unblinded_token) != unblinded_tokens.cend();
      });

  unblinded_tokens_.erase(iter, unblinded_tokens_.cend());
}

void UnblindedTokens::RemoveAllTokens() {
  unblinded_tokens_.clear();
}

bool UnblindedTokens::TokenExists(const UnblindedTokenInfo& unblinded_token) {
  auto iter = std::find_if(unblinded_tokens_.cbegin(), unblinded_tokens_.cend(),
                           [&unblinded_token](const UnblindedTokenInfo& value) {
                             return unblinded_token == value;
                           });

  return iter != unblinded_tokens_.cend();
}

int UnblindedTokens::Count() const {
  return unblinded_tokens_.size();
}

bool UnblindedTokens::IsEmpty() const {
  return unblinded_tokens_.empty();
}

}  // namespace privacy
}  // namespace ads
