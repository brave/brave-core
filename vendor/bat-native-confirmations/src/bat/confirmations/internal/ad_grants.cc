/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/confirmations/internal/ad_grants.h"
#include "bat/confirmations/internal/logging.h"
#include "bat/confirmations/internal/confirmations_impl.h"

#include "base/json/json_reader.h"
#include "third_party/re2/src/re2/re2.h"

namespace confirmations {

AdGrants::AdGrants(
    ConfirmationsImpl* confirmations,
    ConfirmationsClient* confirmations_client) :
    balance_(0.0),
    confirmations_(confirmations),
    confirmations_client_(confirmations_client) {
  (void)confirmations_;
  (void)confirmations_client_;
}

AdGrants::~AdGrants() = default;

bool AdGrants::SetFromJson(const std::string& json) {
  base::Optional<base::Value> value = base::JSONReader::Read(json);
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

bool AdGrants::SetFromDictionary(base::DictionaryValue* dictionary) {
  DCHECK(dictionary);

  auto* value = dictionary->FindKey("grants_balance");
  if (!value || !value->is_double()) {
    return false;
  }

  balance_ = value->GetDouble();

  return true;
}

double AdGrants::GetBalance() const {
  return balance_;
}

///////////////////////////////////////////////////////////////////////////////

bool AdGrants::GetAmountFromDictionary(
    base::DictionaryValue* dictionary,
    double* amount) const {
  DCHECK(dictionary);
  DCHECK(amount);

  auto* value = dictionary->FindKey("amount");
  if (!value || !value->is_string()) {
    return false;
  }

  auto amount_value = value->GetString();

  // Match a double, i.e. 1.23
  if (!re2::RE2::FullMatch(amount_value, "[+]?([0-9]*[.])?[0-9]+")) {
    return false;
  }

  *amount = std::stod(amount_value);

  return true;
}

}  // namespace confirmations
