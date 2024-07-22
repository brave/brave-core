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
#include "brave/components/brave_ads/core/internal/account/issuers/issuer_info.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuer_types.h"

namespace brave_ads {

namespace {

constexpr char kIssuerTypeKey[] = "name";
constexpr char kIssuerPublicKeysKey[] = "publicKeys";
constexpr char kIssuerPublicKeyKey[] = "publicKey";
constexpr char kIssuerAssociatedValueKey[] = "associatedValue";

constexpr char kUndefinedIssuerType[] = "";
constexpr char kConfirmationsIssuerType[] = "confirmations";
constexpr char kPaymentsIssuerType[] = "payments";

std::optional<std::string> ToString(const IssuerType issuer_type) {
  switch (issuer_type) {
    case IssuerType::kUndefined: {
      return std::nullopt;
    }

    case IssuerType::kConfirmations: {
      return kConfirmationsIssuerType;
    }

    case IssuerType::kPayments: {
      return kPaymentsIssuerType;
    }
  }

  NOTREACHED_NORETURN() << "Unexpected value for IssuerType: "
                        << base::to_underlying(issuer_type);
}

std::optional<IssuerType> ParseIssuerType(const base::Value::Dict& dict) {
  const std::string* const issuer_type = dict.FindString(kIssuerTypeKey);
  if (!issuer_type) {
    return std::nullopt;
  }

  if (*issuer_type == kUndefinedIssuerType) {
    return IssuerType::kUndefined;
  }

  if (*issuer_type == kConfirmationsIssuerType) {
    return IssuerType::kConfirmations;
  }

  if (*issuer_type == kPaymentsIssuerType) {
    return IssuerType::kPayments;
  }

  return std::nullopt;
}

std::optional<IssuerPublicKeyMap> ParseIssuerPublicKeys(
    const base::Value::Dict& dict) {
  const auto* const list = dict.FindList(kIssuerPublicKeysKey);
  if (!list) {
    return std::nullopt;
  }

  IssuerPublicKeyMap issuer_public_keys;
  for (const auto& value : *list) {
    const auto* const issuer_public_key_dict = value.GetIfDict();
    if (!issuer_public_key_dict) {
      return std::nullopt;
    }

    const std::string* const issuer_public_key =
        issuer_public_key_dict->FindString(kIssuerPublicKeyKey);
    if (!issuer_public_key) {
      return std::nullopt;
    }

    const std::string* const issuer_associated_value =
        issuer_public_key_dict->FindString(kIssuerAssociatedValueKey);
    if (!issuer_associated_value) {
      return std::nullopt;
    }
    double issuer_associated_value_as_double;
    if (!base::StringToDouble(*issuer_associated_value,
                              &issuer_associated_value_as_double)) {
      // TODO(https://github.com/brave/brave-browser/issues/33546): Decouple
      // payment and confirmation issuer structs/parsing so that we do not need
      // to set the associated value to 0 when an "associatedValue" key has an
      // empty value.
      issuer_associated_value_as_double = 0.0;
    }

    issuer_public_keys.insert(
        {*issuer_public_key, issuer_associated_value_as_double});
  }

  return issuer_public_keys;
}

}  // namespace

base::Value::List IssuersToValue(const IssuerList& issuers) {
  base::Value::List list;

  for (const auto& issuer : issuers) {
    const std::optional<std::string> issuer_type = ToString(issuer.type);
    if (!issuer_type) {
      continue;
    }

    base::Value::List issuer_public_keys_list;
    for (const auto& [issuer_public_key, issuer_associated_value] :
         issuer.public_keys) {
      issuer_public_keys_list.Append(
          base::Value::Dict()
              .Set(kIssuerPublicKeyKey, issuer_public_key)
              .Set(kIssuerAssociatedValueKey,
                   base::NumberToString(issuer_associated_value)));
    }

    list.Append(
        base::Value::Dict()
            .Set(kIssuerTypeKey, *issuer_type)
            .Set(kIssuerPublicKeysKey, std::move(issuer_public_keys_list)));
  }

  return list;
}

std::optional<IssuerList> ValueToIssuers(const base::Value::List& list) {
  IssuerList issuers;
  issuers.reserve(list.size());

  for (const auto& value : list) {
    const auto* const dict = value.GetIfDict();
    if (!dict) {
      continue;
    }

    const std::optional<IssuerType> issuer_type = ParseIssuerType(*dict);
    if (!issuer_type) {
      return std::nullopt;
    }
    CHECK_NE(IssuerType::kUndefined, *issuer_type);

    const std::optional<IssuerPublicKeyMap> issuer_public_keys =
        ParseIssuerPublicKeys(*dict);
    if (!issuer_public_keys) {
      return std::nullopt;
    }

    IssuerInfo issuer;
    issuer.type = *issuer_type;
    issuer.public_keys = *issuer_public_keys;

    issuers.push_back(issuer);
  }

  return issuers;
}

}  // namespace brave_ads
