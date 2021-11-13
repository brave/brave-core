/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed w
 * h this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/privacy/unblinded_payment_tokens/unblinded_payment_tokens.h"

#include <string>
#include <utility>

#include "base/check_op.h"
#include "base/guid.h"
#include "base/values.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/privacy/unblinded_payment_tokens/unblinded_payment_token_info.h"

namespace ads {
namespace privacy {

UnblindedPaymentTokens::UnblindedPaymentTokens() = default;

UnblindedPaymentTokens::~UnblindedPaymentTokens() = default;

UnblindedPaymentTokenInfo UnblindedPaymentTokens::GetToken() const {
  DCHECK_NE(Count(), 0);

  return unblinded_payment_tokens_.front();
}

UnblindedPaymentTokenList UnblindedPaymentTokens::GetAllTokens() const {
  return unblinded_payment_tokens_;
}

base::Value UnblindedPaymentTokens::GetTokensAsList() {
  base::Value list(base::Value::Type::LIST);

  for (const auto& unblinded_payment_token : unblinded_payment_tokens_) {
    base::Value dictionary(base::Value::Type::DICTIONARY);

    dictionary.SetKey(
        "transaction_id",
        base::Value(std::string(unblinded_payment_token.transaction_id)));

    dictionary.SetKey(
        "unblinded_token",
        base::Value(unblinded_payment_token.value.encode_base64()));

    dictionary.SetKey(
        "public_key",
        base::Value(unblinded_payment_token.public_key.encode_base64()));

    dictionary.SetKey(
        "confirmation_type",
        base::Value(std::string(unblinded_payment_token.confirmation_type)));

    dictionary.SetKey(
        "ad_type", base::Value(std::string(unblinded_payment_token.ad_type)));

    list.Append(std::move(dictionary));
  }

  return list;
}

void UnblindedPaymentTokens::SetTokens(
    const UnblindedPaymentTokenList& unblinded_payment_tokens) {
  unblinded_payment_tokens_ = unblinded_payment_tokens;
}

void UnblindedPaymentTokens::SetTokensFromList(const base::Value& list) {
  UnblindedPaymentTokenList unblinded_payment_tokens;

  for (const auto& value : list.GetList()) {
    const base::DictionaryValue* dictionary = nullptr;
    if (!value.GetAsDictionary(&dictionary)) {
      BLOG(0, "Unblinded payment token should be a dictionary");
      continue;
    }

    UnblindedPaymentTokenInfo unblinded_payment_token;

    // Transaction id
    const std::string* transaction_id_value =
        dictionary->FindStringKey("transaction_id");
    if (!transaction_id_value) {
      // Migrate legacy confirmations
      unblinded_payment_token.transaction_id = base::GenerateGUID();
    } else {
      unblinded_payment_token.transaction_id = *transaction_id_value;
    }

    // Unblinded payment token
    const std::string* unblinded_payment_token_value =
        dictionary->FindStringKey("unblinded_token");
    if (!unblinded_payment_token_value) {
      BLOG(
          0,
          "Unblinded payment token dictionary missing unblinded payment token");
      continue;
    }
    unblinded_payment_token.value =
        UnblindedToken::decode_base64(*unblinded_payment_token_value);

    // Public key
    const std::string* public_key_value =
        dictionary->FindStringKey("public_key");
    if (!public_key_value) {
      BLOG(0, "Unblinded payment token dictionary missing public_key");
      continue;
    }
    unblinded_payment_token.public_key =
        PublicKey::decode_base64(*public_key_value);

    // Confirmation type
    const std::string* confirmation_type_value =
        dictionary->FindStringKey("confirmation_type");
    if (confirmation_type_value) {
      unblinded_payment_token.confirmation_type =
          ConfirmationType(*confirmation_type_value);
    }

    // Ad type
    const std::string* ad_type_value = dictionary->FindStringKey("ad_type");
    if (ad_type_value) {
      unblinded_payment_token.ad_type = AdType(*ad_type_value);
    }

    unblinded_payment_tokens.push_back(unblinded_payment_token);
  }

  SetTokens(unblinded_payment_tokens);
}

void UnblindedPaymentTokens::AddTokens(
    const UnblindedPaymentTokenList& unblinded_payment_tokens) {
  for (const auto& unblinded_payment_token : unblinded_payment_tokens) {
    if (TokenExists(unblinded_payment_token)) {
      continue;
    }

    unblinded_payment_tokens_.push_back(unblinded_payment_token);
  }
}

bool UnblindedPaymentTokens::RemoveToken(
    const UnblindedPaymentTokenInfo& unblinded_payment_token) {
  auto iter = std::find_if(
      unblinded_payment_tokens_.cbegin(), unblinded_payment_tokens_.cend(),
      [&unblinded_payment_token](const UnblindedPaymentTokenInfo& value) {
        return unblinded_payment_token == value;
      });

  if (iter == unblinded_payment_tokens_.end()) {
    return false;
  }

  unblinded_payment_tokens_.erase(iter);

  return true;
}

void UnblindedPaymentTokens::RemoveTokens(
    const UnblindedPaymentTokenList& unblinded_payment_tokens) {
  const auto iter = std::remove_if(
      unblinded_payment_tokens_.begin(), unblinded_payment_tokens_.end(),
      [&unblinded_payment_tokens](
          const UnblindedPaymentTokenInfo& unblinded_payment_token) {
        return std::find(unblinded_payment_tokens.cbegin(),
                         unblinded_payment_tokens.cend(),
                         unblinded_payment_token) !=
               unblinded_payment_tokens.end();
      });

  unblinded_payment_tokens_.erase(iter, unblinded_payment_tokens_.end());
}

void UnblindedPaymentTokens::RemoveAllTokens() {
  unblinded_payment_tokens_.clear();
}

bool UnblindedPaymentTokens::TokenExists(
    const UnblindedPaymentTokenInfo& unblinded_payment_token) {
  auto iter = std::find_if(
      unblinded_payment_tokens_.cbegin(), unblinded_payment_tokens_.cend(),
      [&unblinded_payment_token](const UnblindedPaymentTokenInfo& value) {
        return unblinded_payment_token == value;
      });

  if (iter == unblinded_payment_tokens_.end()) {
    return false;
  }

  return true;
}

int UnblindedPaymentTokens::Count() const {
  return unblinded_payment_tokens_.size();
}

bool UnblindedPaymentTokens::IsEmpty() const {
  return unblinded_payment_tokens_.empty();
}

}  // namespace privacy
}  // namespace ads
