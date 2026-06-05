/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/issuers/token_issuers/token_issuer_value_util.h"

#include <string>
#include <string_view>

#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_ads/core/internal/account/issuers/token_issuers/confirmation_token_issuer_info.h"
#include "brave/components/brave_ads/core/internal/account/issuers/token_issuers/payment_token_issuer_info.h"

namespace brave_ads {

namespace {

constexpr std::string_view kTokenIssuerTypeKey = "name";
constexpr std::string_view kTokenIssuerPublicKeysKey = "publicKeys";
constexpr std::string_view kTokenIssuerPublicKeyKey = "publicKey";
constexpr std::string_view kTokenIssuerAssociatedValueKey = "associatedValue";

constexpr char kConfirmationTokenIssuerType[] = "confirmations";
constexpr char kPaymentTokenIssuerType[] = "payments";

enum class TokenIssuerType { kConfirmations, kPayments };

std::optional<TokenIssuerType> ParseTokenIssuerType(
    const base::DictValue& dict) {
  const std::string* const token_issuer_type =
      dict.FindString(kTokenIssuerTypeKey);
  if (!token_issuer_type) {
    return std::nullopt;
  }

  if (*token_issuer_type == kConfirmationTokenIssuerType) {
    return TokenIssuerType::kConfirmations;
  }

  if (*token_issuer_type == kPaymentTokenIssuerType) {
    return TokenIssuerType::kPayments;
  }

  return std::nullopt;
}

std::optional<ConfirmationTokenIssuerInfo> ParseConfirmationTokenIssuer(
    const base::DictValue& dict) {
  const auto* const list = dict.FindList(kTokenIssuerPublicKeysKey);
  if (!list) {
    return std::nullopt;
  }

  ConfirmationTokenIssuerInfo issuer;
  for (const auto& value : *list) {
    const auto* const key_dict = value.GetIfDict();
    if (!key_dict) {
      return std::nullopt;
    }

    const std::string* const public_key =
        key_dict->FindString(kTokenIssuerPublicKeyKey);
    if (!public_key) {
      return std::nullopt;
    }

    issuer.public_keys.insert(*public_key);
  }

  return issuer;
}

std::optional<PaymentTokenIssuerInfo> ParsePaymentTokenIssuer(
    const base::DictValue& dict) {
  const auto* const list = dict.FindList(kTokenIssuerPublicKeysKey);
  if (!list) {
    return std::nullopt;
  }

  PaymentTokenIssuerInfo issuer;
  for (const auto& value : *list) {
    const auto* const key_dict = value.GetIfDict();
    if (!key_dict) {
      return std::nullopt;
    }

    const std::string* const public_key =
        key_dict->FindString(kTokenIssuerPublicKeyKey);
    if (!public_key) {
      return std::nullopt;
    }

    const std::string* const associated_value_str =
        key_dict->FindString(kTokenIssuerAssociatedValueKey);
    if (!associated_value_str) {
      return std::nullopt;
    }

    double associated_value;
    if (!base::StringToDouble(*associated_value_str, &associated_value)) {
      return std::nullopt;
    }

    issuer.public_keys.insert({*public_key, associated_value});
  }

  return issuer;
}

}  // namespace

base::ListValue TokenIssuersToList(
    const ConfirmationTokenIssuerInfo& confirmation,
    const PaymentTokenIssuerInfo& payment) {
  base::ListValue list;

  {
    base::ListValue public_keys_list;
    for (const auto& public_key : confirmation.public_keys) {
      public_keys_list.Append(base::DictValue()
                                  .Set(kTokenIssuerPublicKeyKey, public_key)
                                  .Set(kTokenIssuerAssociatedValueKey, ""));
    }
    list.Append(
        base::DictValue()
            .Set(kTokenIssuerTypeKey, kConfirmationTokenIssuerType)
            .Set(kTokenIssuerPublicKeysKey, std::move(public_keys_list)));
  }

  {
    base::ListValue public_keys_list;
    for (const auto& [public_key, associated_value] : payment.public_keys) {
      public_keys_list.Append(base::DictValue()
                                  .Set(kTokenIssuerPublicKeyKey, public_key)
                                  .Set(kTokenIssuerAssociatedValueKey,
                                       base::NumberToString(associated_value)));
    }
    list.Append(
        base::DictValue()
            .Set(kTokenIssuerTypeKey, kPaymentTokenIssuerType)
            .Set(kTokenIssuerPublicKeysKey, std::move(public_keys_list)));
  }

  return list;
}

std::optional<TokenIssuersInfo> MaybeBuildTokenIssuersFromList(
    const base::ListValue& list) {
  std::optional<ConfirmationTokenIssuerInfo> confirmation;
  std::optional<PaymentTokenIssuerInfo> payment;

  for (const auto& value : list) {
    const auto* const dict = value.GetIfDict();
    if (!dict) {
      return std::nullopt;
    }

    std::optional<TokenIssuerType> type = ParseTokenIssuerType(*dict);
    if (!type) {
      continue;
    }

    switch (*type) {
      case TokenIssuerType::kConfirmations: {
        if (confirmation) {
          return std::nullopt;
        }
        confirmation = ParseConfirmationTokenIssuer(*dict);
        if (!confirmation) {
          return std::nullopt;
        }
        break;
      }

      case TokenIssuerType::kPayments: {
        if (payment) {
          return std::nullopt;
        }
        payment = ParsePaymentTokenIssuer(*dict);
        if (!payment) {
          return std::nullopt;
        }
        break;
      }
    }
  }

  if (!confirmation || !payment) {
    return std::nullopt;
  }

  return TokenIssuersInfo{std::move(*confirmation), std::move(*payment)};
}

}  // namespace brave_ads
