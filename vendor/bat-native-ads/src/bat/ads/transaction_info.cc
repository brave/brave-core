/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/transaction_info.h"

#include "base/check.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "bat/ads/internal/number_util.h"

namespace ads {

TransactionInfo::TransactionInfo() = default;

TransactionInfo::TransactionInfo(const TransactionInfo& info) = default;

TransactionInfo::~TransactionInfo() = default;

bool TransactionInfo::operator==(const TransactionInfo& rhs) const {
  return id == rhs.id && DoubleEquals(created_at, rhs.created_at) &&
         creative_instance_id == rhs.creative_instance_id &&
         DoubleEquals(value, rhs.value) && ad_type == rhs.ad_type &&
         confirmation_type == rhs.confirmation_type &&
         DoubleEquals(reconciled_at, rhs.reconciled_at);
}

bool TransactionInfo::operator!=(const TransactionInfo& rhs) const {
  return !(*this == rhs);
}

bool TransactionInfo::IsValid() const {
  if (id.empty() || creative_instance_id.empty() ||
      ad_type == AdType::kUndefined ||
      confirmation_type == ConfirmationType::kUndefined ||
      DoubleEquals(created_at, 0.0)) {
    return false;
  }

  return true;
}

void TransactionInfo::ToDictionary(base::Value* dictionary) const {
  DCHECK(dictionary);

  dictionary->SetStringKey("id", id);

  dictionary->SetStringKey("timestamp_in_seconds",
                           base::NumberToString(created_at));

  dictionary->SetDoubleKey("estimated_redemption_value", value);

  dictionary->SetStringKey("confirmation_type", std::string(confirmation_type));

  dictionary->SetStringKey("reconciled_at",
                           base::NumberToString(reconciled_at));
}

void TransactionInfo::FromDictionary(base::DictionaryValue* dictionary) {
  DCHECK(dictionary);

  // Id
  const std::string* id_value = dictionary->FindStringKey("id");
  if (id_value) {
    id = *id_value;
  }

  // Created at
  const std::string* created_at_value =
      dictionary->FindStringKey("timestamp_in_seconds");
  if (created_at_value) {
    base::StringToDouble(*created_at_value, &created_at);
  }

  // Estimated redemption value
  value = dictionary->FindDoubleKey("value").value_or(0.0);

  // Confirmation type
  const std::string* confirmation_type_value =
      dictionary->FindStringKey("confirmation_type");
  if (confirmation_type_value) {
    confirmation_type = ConfirmationType(*confirmation_type_value);
  }

  // Reconciled at
  const std::string* reconciled_at_value =
      dictionary->FindStringKey("reconciled_at");
  if (reconciled_at_value) {
    base::StringToDouble(*reconciled_at_value, &reconciled_at);
  }
}

}  // namespace ads
