/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/catalog/catalog_issuers_info.h"

#include <utility>

#include "base/strings/string_number_conversions.h"
#include "bat/ads/internal/logging.h"
#include "third_party/re2/src/re2/re2.h"

namespace ads {

CatalogIssuersInfo::CatalogIssuersInfo() = default;

CatalogIssuersInfo::CatalogIssuersInfo(const CatalogIssuersInfo& info) =
    default;

CatalogIssuersInfo::~CatalogIssuersInfo() = default;

bool CatalogIssuersInfo::operator==(const CatalogIssuersInfo& rhs) const {
  return public_key == rhs.public_key && issuers == rhs.issuers;
}

bool CatalogIssuersInfo::operator!=(const CatalogIssuersInfo& rhs) const {
  return !(*this == rhs);
}

base::Value CatalogIssuersInfo::ToDictionary() const {
  base::Value dictionary(base::Value::Type::DICTIONARY);
  dictionary.SetKey("public_key", base::Value(public_key));

  base::Value list(base::Value::Type::LIST);

  for (const auto& issuer : issuers) {
    base::Value issuer_dictionary(base::Value::Type::DICTIONARY);

    issuer_dictionary.SetKey("name", base::Value(issuer.name));
    issuer_dictionary.SetKey("public_key", base::Value(issuer.public_key));

    list.Append(std::move(issuer_dictionary));
  }

  dictionary.SetKey("issuers", base::Value(std::move(list)));

  return dictionary;
}

bool CatalogIssuersInfo::FromDictionary(base::Value* dictionary) {
  DCHECK(dictionary);

  // Public key
  const std::string* public_key_value = dictionary->FindStringKey("public_key");
  if (!public_key_value) {
    return false;
  }

  public_key = *public_key_value;

  // Issuers
  const base::Value* issuers_list = dictionary->FindListKey("issuers");
  if (!issuers_list) {
    return false;
  }

  CatalogIssuerList catalog_issuers;

  for (const auto& value : issuers_list->GetList()) {
    const base::DictionaryValue* issuer_dictionary = nullptr;
    if (!value.GetAsDictionary(&issuer_dictionary)) {
      return false;
    }

    // Public key
    const std::string* public_key =
        issuer_dictionary->FindStringKey("public_key");
    if (!public_key) {
      return false;
    }

    // Name
    const std::string* name = issuer_dictionary->FindStringKey("name");
    if (!name) {
      return false;
    }

    CatalogIssuerInfo catalog_issuer;
    catalog_issuer.name = *name;
    catalog_issuer.public_key = *public_key;

    catalog_issuers.push_back(catalog_issuer);
  }

  issuers = catalog_issuers;

  return true;
}

bool CatalogIssuersInfo::IsValid() const {
  if (public_key.empty() || issuers.empty()) {
    return false;
  }

  return true;
}

bool CatalogIssuersInfo::PublicKeyExists(const std::string& public_key) const {
  if (this->public_key == public_key) {
    return true;
  }

  const auto iter =
      std::find_if(issuers.begin(), issuers.end(),
                   [&public_key](const CatalogIssuerInfo& issuer) {
                     return issuer.public_key == public_key;
                   });

  if (iter == issuers.end()) {
    return false;
  }

  return true;
}

absl::optional<double> CatalogIssuersInfo::GetEstimatedRedemptionValue(
    const std::string& public_key) const {
  const auto iter = std::find_if(issuers.begin(), issuers.end(),
                                 [&public_key](const auto& issuer) {
                                   return issuer.public_key == public_key;
                                 });

  if (iter == issuers.end()) {
    return absl::nullopt;
  }

  const CatalogIssuerInfo catalog_issuer = *iter;

  std::string name = catalog_issuer.name;

  if (!re2::RE2::Replace(&name, "BAT", "")) {
    BLOG(1,
         "Failed to get estimated redemption value due to invalid catalog "
         "issuer name");

    return absl::nullopt;
  }

  double estimated_redemption_value;
  if (!base::StringToDouble(name, &estimated_redemption_value)) {
    BLOG(1,
         "Failed to get estimated redemption value due to invalid catalog "
         "issuer name");

    return absl::nullopt;
  }

  return estimated_redemption_value;
}

}  // namespace ads
