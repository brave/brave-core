/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/ad_rewards/ad_grants/ad_grants.h"

#include "base/json/json_reader.h"
#include "base/strings/string_number_conversions.h"
#include "bat/ads/internal/logging.h"
#include "third_party/re2/src/re2/re2.h"

namespace ads {

AdGrants::AdGrants() = default;

AdGrants::~AdGrants() = default;

bool AdGrants::SetFromJson(const std::string& json) {
  absl::optional<base::Value> value = base::JSONReader::Read(json);
  if (!value || !value->is_dict()) {
    return false;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    return false;
  }

  if (!GetAmountFromDictionary(dictionary, &balance_)) {
    return false;
  }

  return true;
}

bool AdGrants::SetFromDictionary(base::Value* dictionary) {
  DCHECK(dictionary);

  const absl::optional<double> balance =
      dictionary->FindDoubleKey("grants_balance");
  if (!balance) {
    return false;
  }
  balance_ = *balance;

  return true;
}

double AdGrants::GetBalance() const {
  return balance_;
}

///////////////////////////////////////////////////////////////////////////////

bool AdGrants::GetAmountFromDictionary(base::DictionaryValue* dictionary,
                                       double* amount) const {
  DCHECK(dictionary);
  DCHECK(amount);

  const std::string* value = dictionary->FindStringKey("amount");
  if (!value) {
    return false;
  }

  // Match a double, i.e. 1.23
  if (!re2::RE2::FullMatch(*value, "[+]?([0-9]*[.])?[0-9]+")) {
    return false;
  }

  if (!base::StringToDouble(*value, amount)) {
    return false;
  }

  return true;
}

}  // namespace ads
