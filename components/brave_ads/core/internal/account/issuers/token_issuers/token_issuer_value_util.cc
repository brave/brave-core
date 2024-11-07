/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/issuers/token_issuers/token_issuer_value_util.h"

#include <string>
#include <utility>

#include "base/check_op.h"
#include "base/notreached.h"
#include "base/strings/string_number_conversions.h"
#include "base/types/cxx23_to_underlying.h"
#include "brave/components/brave_ads/core/internal/account/issuers/token_issuers/token_issuer_info.h"
#include "brave/components/brave_ads/core/internal/account/issuers/token_issuers/token_issuer_types.h"

namespace brave_ads {

namespace {

constexpr char kTokenIssuerTypeKey[] = "name";
constexpr char kTokenIssuerPublicKeysKey[] = "publicKeys";
constexpr char kTokenIssuerPublicKeyKey[] = "publicKey";
constexpr char kTokenIssuerAssociatedValueKey[] = "associatedValue";

constexpr char kUndefinedTokenIssuerType[] = "";
constexpr char kConfirmationTokenIssuerType[] = "confirmations";
constexpr char kPaymentTokenIssuerType[] = "payments";

std::optional<std::string> ToString(const TokenIssuerType token_issuer_type) {
  switch (token_issuer_type) {
    case TokenIssuerType::kUndefined: {
      return std::nullopt;
    }

    case TokenIssuerType::kConfirmations: {
      return kConfirmationTokenIssuerType;
    }

    case TokenIssuerType::kPayments: {
      return kPaymentTokenIssuerType;
    }
  }

  NOTREACHED() << "Unexpected value for TokenIssuerType: "
               << base::to_underlying(token_issuer_type);
}

std::optional<TokenIssuerType> ParseTokenIssuerType(
    const base::Value::Dict& dict) {
  const std::string* const token_issuer_type =
      dict.FindString(kTokenIssuerTypeKey);
  if (!token_issuer_type) {
    return std::nullopt;
  }

  if (*token_issuer_type == kUndefinedTokenIssuerType) {
    return TokenIssuerType::kUndefined;
  }

  if (*token_issuer_type == kConfirmationTokenIssuerType) {
    return TokenIssuerType::kConfirmations;
  }

  if (*token_issuer_type == kPaymentTokenIssuerType) {
    return TokenIssuerType::kPayments;
  }

  return std::nullopt;
}

std::optional<TokenIssuerPublicKeyMap> ParseTokenIssuerPublicKeys(
    const base::Value::Dict& dict) {
  const auto* const list = dict.FindList(kTokenIssuerPublicKeysKey);
  if (!list) {
    return std::nullopt;
  }

  TokenIssuerPublicKeyMap token_issuer_public_keys;
  for (const auto& value : *list) {
    const auto* const token_issuer_public_key_dict = value.GetIfDict();
    if (!token_issuer_public_key_dict) {
      return std::nullopt;
    }

    const std::string* const token_issuer_public_key =
        token_issuer_public_key_dict->FindString(kTokenIssuerPublicKeyKey);
    if (!token_issuer_public_key) {
      return std::nullopt;
    }

    const std::string* const token_issuer_associated_value =
        token_issuer_public_key_dict->FindString(
            kTokenIssuerAssociatedValueKey);
    if (!token_issuer_associated_value) {
      return std::nullopt;
    }
    double token_issuer_associated_value_as_double;
    if (!base::StringToDouble(*token_issuer_associated_value,
                              &token_issuer_associated_value_as_double)) {
      // TODO(https://github.com/brave/brave-browser/issues/33546): Decouple
      // payment and confirmation issuer structs/parsing so that we do not need
      // to set the associated value to 0 when an "associatedValue" key has an
      // empty value.
      token_issuer_associated_value_as_double = 0.0;
    }

    token_issuer_public_keys.insert(
        {*token_issuer_public_key, token_issuer_associated_value_as_double});
  }

  return token_issuer_public_keys;
}

}  // namespace

base::Value::List TokenIssuersToValue(const TokenIssuerList& token_issuers) {
  base::Value::List list;

  for (const auto& token_issuer : token_issuers) {
    const std::optional<std::string> token_issuer_type =
        ToString(token_issuer.type);
    if (!token_issuer_type) {
      continue;
    }

    base::Value::List token_issuer_public_keys_list;
    for (const auto& [public_key, associated_value] :
         token_issuer.public_keys) {
      token_issuer_public_keys_list.Append(
          base::Value::Dict()
              .Set(kTokenIssuerPublicKeyKey, public_key)
              .Set(kTokenIssuerAssociatedValueKey,
                   base::NumberToString(associated_value)));
    }

    list.Append(base::Value::Dict()
                    .Set(kTokenIssuerTypeKey, *token_issuer_type)
                    .Set(kTokenIssuerPublicKeysKey,
                         std::move(token_issuer_public_keys_list)));
  }

  return list;
}

std::optional<TokenIssuerList> TokenIssuersFromValue(
    const base::Value::List& list) {
  TokenIssuerList token_issuers;
  token_issuers.reserve(list.size());

  for (const auto& value : list) {
    const auto* const dict = value.GetIfDict();
    if (!dict) {
      return std::nullopt;
    }

    const std::optional<TokenIssuerType> token_issuer_type =
        ParseTokenIssuerType(*dict);
    if (!token_issuer_type) {
      return std::nullopt;
    }
    CHECK_NE(TokenIssuerType::kUndefined, *token_issuer_type);

    const std::optional<TokenIssuerPublicKeyMap> token_issuer_public_keys =
        ParseTokenIssuerPublicKeys(*dict);
    if (!token_issuer_public_keys) {
      return std::nullopt;
    }

    TokenIssuerInfo token_issuer;
    token_issuer.type = *token_issuer_type;
    token_issuer.public_keys = *token_issuer_public_keys;

    token_issuers.push_back(token_issuer);
  }

  return token_issuers;
}

}  // namespace brave_ads
