/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/issuers/issuers_value_util.h"

#include <ostream>
#include <string>
#include <utility>

#include "absl/types/optional.h"
#include "base/check_op.h"
#include "base/notreached.h"
#include "base/strings/string_number_conversions.h"
#include "bat/ads/internal/account/issuers/issuer_types.h"
#include "bat/ads/internal/account/issuers/public_key_alias.h"

namespace ads {

namespace {

constexpr char kNameKey[] = "name";
constexpr char kPublicKeysKey[] = "publicKeys";
constexpr char kPublicKeyKey[] = "publicKey";
constexpr char kAssociatedValueKey[] = "associatedValue";

constexpr char kUndefinedName[] = "";
constexpr char kConfirmationsName[] = "confirmations";
constexpr char kPaymentsName[] = "payments";

absl::optional<std::string> GetNameForIssuerType(const IssuerType type) {
  switch (type) {
    case IssuerType::kUndefined: {
      return absl::nullopt;
    }

    case IssuerType::kConfirmations: {
      return kConfirmationsName;
    }

    case IssuerType::kPayments: {
      return kPaymentsName;
    }
  }

  NOTREACHED() << "Unexpected value for IssuerType: " << static_cast<int>(type);
  return absl::nullopt;
}

absl::optional<IssuerType> ParseIssuerType(const base::Value::Dict& value) {
  const std::string* const name = value.FindString(kNameKey);
  if (!name) {
    return absl::nullopt;
  }

  if (*name == kUndefinedName) {
    return IssuerType::kUndefined;
  }

  if (*name == kConfirmationsName) {
    return IssuerType::kConfirmations;
  }

  if (*name == kPaymentsName) {
    return IssuerType::kPayments;
  }

  return absl::nullopt;
}

absl::optional<PublicKeyMap> ParsePublicKeys(const base::Value::Dict& value) {
  const base::Value::List* const list = value.FindList(kPublicKeysKey);
  if (!list) {
    return absl::nullopt;
  }

  PublicKeyMap public_keys;
  for (const auto& item : *list) {
    const base::Value::Dict* const dict = item.GetIfDict();
    if (!dict) {
      return absl::nullopt;
    }

    const std::string* const public_key = dict->FindString(kPublicKeyKey);
    if (!public_key) {
      return absl::nullopt;
    }

    const std::string* const associated_value =
        dict->FindString(kAssociatedValueKey);
    if (!associated_value) {
      return absl::nullopt;
    }
    double associated_value_as_double = 0.0;
    base::StringToDouble(*associated_value, &associated_value_as_double);

    public_keys.insert({*public_key, associated_value_as_double});
  }

  return public_keys;
}

}  // namespace

base::Value::List IssuersToValue(const IssuerList& issuers) {
  base::Value::List list;

  for (const auto& issuer : issuers) {
    const absl::optional<std::string> name = GetNameForIssuerType(issuer.type);
    if (!name) {
      continue;
    }

    base::Value::Dict dict;

    dict.Set(kNameKey, *name);

    base::Value::List public_keys;
    for (const auto& [key, value] : issuer.public_keys) {
      base::Value::Dict public_key_dict;
      public_key_dict.Set(kPublicKeyKey, key);
      public_key_dict.Set(kAssociatedValueKey, base::NumberToString(value));
      public_keys.Append(std::move(public_key_dict));
    }
    dict.Set(kPublicKeysKey, std::move(public_keys));

    list.Append(std::move(dict));
  }

  return list;
}

absl::optional<IssuerList> ValueToIssuers(const base::Value::List& value) {
  IssuerList issuers;

  for (const auto& item : value) {
    if (!item.is_dict()) {
      continue;
    }

    const base::Value::Dict& dict = item.GetDict();
    const absl::optional<IssuerType> issuer_type = ParseIssuerType(dict);
    if (!issuer_type) {
      return absl::nullopt;
    }
    DCHECK_NE(IssuerType::kUndefined, *issuer_type);

    const absl::optional<PublicKeyMap> public_keys = ParsePublicKeys(dict);
    if (!public_keys) {
      return absl::nullopt;
    }

    IssuerInfo issuer;
    issuer.type = *issuer_type;
    issuer.public_keys = *public_keys;

    issuers.push_back(issuer);
  }

  return issuers;
}

}  // namespace ads
