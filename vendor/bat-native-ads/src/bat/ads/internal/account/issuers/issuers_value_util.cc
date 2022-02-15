/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/issuers/issuers_value_util.h"

#include <string>
#include <utility>
#include <vector>

#include "base/check_op.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "bat/ads/internal/account/issuers/issuer_types.h"
#include "bat/ads/internal/account/issuers/public_key_aliases.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

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
}

absl::optional<IssuerType> ParseIssuerType(const base::Value& value) {
  const std::string* const name = value.FindStringKey(kNameKey);
  if (!name) {
    return absl::nullopt;
  }

  if (*name == kUndefinedName) {
    return IssuerType::kUndefined;
  } else if (*name == kConfirmationsName) {
    return IssuerType::kConfirmations;
  } else if (*name == kPaymentsName) {
    return IssuerType::kPayments;
  }

  return absl::nullopt;
}

absl::optional<PublicKeyMap> ParsePublicKeys(const base::Value& value) {
  const base::Value* const public_keys_value =
      value.FindListKey(kPublicKeysKey);
  if (!public_keys_value) {
    return absl::nullopt;
  }

  PublicKeyMap public_keys;
  for (const auto& public_key_value : public_keys_value->GetListDeprecated()) {
    if (!public_key_value.is_dict()) {
      return absl::nullopt;
    }

    const std::string* const public_key =
        public_key_value.FindStringKey(kPublicKeyKey);
    if (!public_key) {
      return absl::nullopt;
    }

    const std::string* const associated_value =
        public_key_value.FindStringKey(kAssociatedValueKey);
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

base::Value IssuerListToValue(const IssuerList& issuers) {
  base::Value issuers_list(base::Value::Type::LIST);

  for (const auto& issuer : issuers) {
    base::Value issuer_dictionary(base::Value::Type::DICTIONARY);

    const absl::optional<std::string>& name_optional =
        GetNameForIssuerType(issuer.type);
    if (!name_optional) {
      continue;
    }
    const std::string& name = name_optional.value();

    issuer_dictionary.SetKey(kNameKey, base::Value(name));

    base::Value public_keys_list(base::Value::Type::LIST);
    for (const auto& public_key : issuer.public_keys) {
      base::Value public_key_dictionary(base::Value::Type::DICTIONARY);

      public_key_dictionary.SetKey(kPublicKeyKey,
                                   base::Value(public_key.first));

      public_key_dictionary.SetKey(
          kAssociatedValueKey,
          base::Value(base::NumberToString(public_key.second)));

      public_keys_list.Append(std::move(public_key_dictionary));
    }
    issuer_dictionary.SetKey(kPublicKeysKey, std::move(public_keys_list));

    issuers_list.Append(std::move(issuer_dictionary));
  }

  return issuers_list;
}

absl::optional<IssuerList> ValueToIssuerList(const base::Value& value) {
  if (!value.is_list()) {
    return absl::nullopt;
  }

  IssuerList issuers;

  for (const auto& issuer_value : value.GetListDeprecated()) {
    if (!issuer_value.is_dict()) {
      return absl::nullopt;
    }

    const absl::optional<IssuerType>& type_optional =
        ParseIssuerType(issuer_value);
    if (!type_optional) {
      return absl::nullopt;
    }
    const IssuerType& type = type_optional.value();
    DCHECK_NE(IssuerType::kUndefined, type);

    const absl::optional<PublicKeyMap>& public_keys_optional =
        ParsePublicKeys(issuer_value);
    if (!public_keys_optional) {
      return absl::nullopt;
    }
    const PublicKeyMap& public_keys = public_keys_optional.value();

    IssuerInfo issuer;
    issuer.type = type;
    issuer.public_keys = public_keys;

    issuers.push_back(issuer);
  }

  return issuers;
}

}  // namespace ads
