/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/issuers/issuers_value_util.h"

#include <string>
#include <utility>

#include "base/check_op.h"
#include "base/notreached.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuer_types.h"
#include "brave/components/brave_ads/core/internal/account/issuers/public_key_alias.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads {

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

  NOTREACHED_NORETURN() << "Unexpected value for IssuerType: "
                        << static_cast<int>(type);
}

absl::optional<IssuerType> ParseIssuerType(const base::Value::Dict& dict) {
  const std::string* const name = dict.FindString(kNameKey);
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

absl::optional<PublicKeyMap> ParsePublicKeys(const base::Value::Dict& dict) {
  const auto* const public_keys_list = dict.FindList(kPublicKeysKey);
  if (!public_keys_list) {
    return absl::nullopt;
  }

  PublicKeyMap public_keys;
  for (const auto& item : *public_keys_list) {
    const auto* const item_dict = item.GetIfDict();
    if (!item_dict) {
      return absl::nullopt;
    }

    const std::string* const public_key = item_dict->FindString(kPublicKeyKey);
    if (!public_key) {
      return absl::nullopt;
    }

    const std::string* const associated_value =
        item_dict->FindString(kAssociatedValueKey);
    if (!associated_value) {
      return absl::nullopt;
    }
    double associated_value_as_double;
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

    base::Value::List public_keys_list;
    for (const auto& [public_key, associated_value] : issuer.public_keys) {
      auto public_key_dict =
          base::Value::Dict()
              .Set(kPublicKeyKey, public_key)
              .Set(kAssociatedValueKey, base::NumberToString(associated_value));

      public_keys_list.Append(std::move(public_key_dict));
    }

    auto dict = base::Value::Dict()
                    .Set(kNameKey, *name)
                    .Set(kPublicKeysKey, std::move(public_keys_list));

    list.Append(std::move(dict));
  }

  return list;
}

absl::optional<IssuerList> ValueToIssuers(const base::Value::List& list) {
  IssuerList issuers;

  for (const auto& item : list) {
    const auto* item_dict = item.GetIfDict();
    if (!item_dict) {
      continue;
    }

    const absl::optional<IssuerType> issuer_type = ParseIssuerType(*item_dict);
    if (!issuer_type) {
      return absl::nullopt;
    }
    CHECK_NE(IssuerType::kUndefined, *issuer_type);

    const absl::optional<PublicKeyMap> public_keys =
        ParsePublicKeys(*item_dict);
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

}  // namespace brave_ads
