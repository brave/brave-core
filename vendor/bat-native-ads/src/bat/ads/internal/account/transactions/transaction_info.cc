/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/transactions/transaction_info.h"

#include "bat/ads/internal/base/numbers/number_util.h"

namespace ads {

TransactionInfo::TransactionInfo() = default;

TransactionInfo::TransactionInfo(const TransactionInfo& info) = default;

TransactionInfo& TransactionInfo::operator=(const TransactionInfo& info) =
    default;

TransactionInfo::~TransactionInfo() = default;

bool TransactionInfo::operator==(const TransactionInfo& rhs) const {
  return id == rhs.id &&
         DoubleEquals(created_at.ToDoubleT(), rhs.created_at.ToDoubleT()) &&
         creative_instance_id == rhs.creative_instance_id &&
         DoubleEquals(value, rhs.value) && ad_type == rhs.ad_type &&
         confirmation_type == rhs.confirmation_type &&
         DoubleEquals(reconciled_at.ToDoubleT(), rhs.reconciled_at.ToDoubleT());
}

bool TransactionInfo::operator!=(const TransactionInfo& rhs) const {
  return !(*this == rhs);
}

bool TransactionInfo::IsValid() const {
  if (id.empty() || creative_instance_id.empty() ||
      ad_type == AdType::kUndefined ||
      confirmation_type == ConfirmationType::kUndefined ||
      created_at.is_null()) {
    return false;
  }

  return true;
}

}  // namespace ads
