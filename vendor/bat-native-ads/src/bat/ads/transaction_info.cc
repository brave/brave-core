/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/transaction_info.h"

#include "base/check.h"
#include "base/strings/string_number_conversions.h"
#include "bat/ads/internal/number_util.h"

namespace ads {

TransactionInfo::TransactionInfo() = default;

TransactionInfo::TransactionInfo(const TransactionInfo& info) = default;

TransactionInfo::~TransactionInfo() = default;

bool TransactionInfo::operator==(const TransactionInfo& rhs) const {
  return timestamp == rhs.timestamp &&
         DoubleEquals(estimated_redemption_value,
                      rhs.estimated_redemption_value) &&
         confirmation_type == rhs.confirmation_type;
}

bool TransactionInfo::operator!=(const TransactionInfo& rhs) const {
  return !(*this == rhs);
}

void TransactionInfo::ToDictionary(base::Value* dictionary) const {
  DCHECK(dictionary);

  dictionary->SetKey("timestamp_in_seconds",
                     base::Value(std::to_string(timestamp)));

  dictionary->SetKey("estimated_redemption_value",
                     base::Value(estimated_redemption_value));

  dictionary->SetKey("confirmation_type", base::Value(confirmation_type));
}

void TransactionInfo::FromDictionary(base::DictionaryValue* dictionary) {
  DCHECK(dictionary);

  // Timestamp
  const std::string* timestamp_value =
      dictionary->FindStringKey("timestamp_in_seconds");
  if (timestamp_value) {
    base::StringToInt64(*timestamp_value, &timestamp);
  }

  // Estimated redemption value
  estimated_redemption_value =
      dictionary->FindDoubleKey("estimated_redemption_value").value_or(0.0);

  // Confirmation type
  const std::string* confirmation_type_value =
      dictionary->FindStringKey("confirmation_type");
  if (confirmation_type_value) {
    confirmation_type = *confirmation_type_value;
  }
}

}  // namespace ads
