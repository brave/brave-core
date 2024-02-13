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
#include "base/types/cxx23_to_underlying.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuer_types.h"
#include "brave/components/brave_ads/core/internal/account/issuers/public_key_alias.h"

namespace brave_ads {

namespace {

constexpr char kNameKey[] = "name";
constexpr char kPublicKeysKey[] = "publicKeys";
constexpr char kPublicKeyKey[] = "publicKey";
constexpr char kAssociatedValueKey[] = "associatedValue";

constexpr char kUndefinedName[] = "";
constexpr char kConfirmationsName[] = "confirmations";
constexpr char kPaymentsName[] = "payments";

std::optional<std::string> GetNameForIssuerType(const IssuerType type) {
  switch (type) {
    case IssuerType::kUndefined: {
      return std::nullopt;
    }

    case IssuerType::kConfirmations: {
      return kConfirmationsName;
    }

    case IssuerType::kPayments: {
      return kPaymentsName;
    }
  }

  NOTREACHED_NORETURN() << "Unexpected value for IssuerType: "
                        << base::to_underlying(type);
}

std::optional<IssuerType> ParseIssuerType(const base::Value::Dict& dict) {
  const std::string* const name = dict.FindString(kNameKey);
  if (!name) {
    return std::nullopt;
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

  return std::nullopt;
}

std::optional<PublicKeyMap> ParsePublicKeys(const base::Value::Dict& dict) {
  const auto* const public_keys_list = dict.FindList(kPublicKeysKey);
  if (!public_keys_list) {
    return std::nullopt;
  }

  PublicKeyMap public_keys;
  for (const auto& item : *public_keys_list) {
    const auto* const item_dict = item.GetIfDict();
    if (!item_dict) {
      return std::nullopt;
    }

    const std::string* const public_key = item_dict->FindString(kPublicKeyKey);
    if (!public_key) {
      return std::nullopt;
    }

    const std::string* const associated_value =
        item_dict->FindString(kAssociatedValueKey);
    if (!associated_value) {
      return std::nullopt;
    }
    double associated_value_as_double;
    if (!base::StringToDouble(*associated_value, &associated_value_as_double)) {
      // TODO(https://github.com/brave/brave-browser/issues/33546): Decouple
      // payment and confirmation issuer structs/parsing so that we do not need
      // to set the associated value to 0 when an "associatedValue" key has an
      // empty value.
      associated_value_as_double = 0.0;
    }

    public_keys.insert({*public_key, associated_value_as_double});
  }

  return public_keys;
}

}  // namespace

base::Value::List IssuersToValue(const IssuerList& issuers) {
  base::Value::List list;

  for (const auto& issuer : issuers) {
    const std::optional<std::string> name = GetNameForIssuerType(issuer.type);
    if (!name) {
      continue;
    }

    base::Value::List public_keys_list;
    for (const auto& [public_key, associated_value] : issuer.public_keys) {
      public_keys_list.Append(base::Value::Dict()
                                  .Set(kPublicKeyKey, public_key)
                                  .Set(kAssociatedValueKey,
                                       base::NumberToString(associated_value)));
    }

    list.Append(base::Value::Dict()
                    .Set(kNameKey, *name)
                    .Set(kPublicKeysKey, std::move(public_keys_list)));
  }

  return list;
}

std::optional<IssuerList> ValueToIssuers(const base::Value::List& list) {
  IssuerList issuers;
  issuers.reserve(list.size());

  for (const auto& item : list) {
    const auto* item_dict = item.GetIfDict();
    if (!item_dict) {
      continue;
    }

    const std::optional<IssuerType> issuer_type = ParseIssuerType(*item_dict);
    if (!issuer_type) {
      return std::nullopt;
    }
    CHECK_NE(IssuerType::kUndefined, *issuer_type);

    const std::optional<PublicKeyMap> public_keys = ParsePublicKeys(*item_dict);
    if (!public_keys) {
      return std::nullopt;
    }

    IssuerInfo issuer;
    issuer.type = *issuer_type;
    issuer.public_keys = *public_keys;

    issuers.push_back(issuer);
  }

  return issuers;
}

}  // namespace brave_ads
