/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/transactions/transaction_info.h"

#include "bat/ads/internal/common/numbers/number_util.h"

namespace ads {

TransactionInfo::TransactionInfo() = default;

TransactionInfo::TransactionInfo(const TransactionInfo& other) = default;

TransactionInfo& TransactionInfo::operator=(const TransactionInfo& other) =
    default;

TransactionInfo::TransactionInfo(TransactionInfo&& other) noexcept = default;

TransactionInfo& TransactionInfo::operator=(TransactionInfo&& other) noexcept =
    default;

TransactionInfo::~TransactionInfo() = default;

bool TransactionInfo::operator==(const TransactionInfo& other) const {
  return id == other.id &&
         DoubleEquals(created_at.ToDoubleT(), other.created_at.ToDoubleT()) &&
         creative_instance_id == other.creative_instance_id &&
         DoubleEquals(value, other.value) && ad_type == other.ad_type &&
         confirmation_type == other.confirmation_type &&
         DoubleEquals(reconciled_at.ToDoubleT(),
                      other.reconciled_at.ToDoubleT());
}

bool TransactionInfo::operator!=(const TransactionInfo& other) const {
  return !(*this == other);
}

bool TransactionInfo::IsValid() const {
  return !(id.empty() || creative_instance_id.empty() ||
           ad_type == AdType::kUndefined ||
           confirmation_type == ConfirmationType::kUndefined ||
           created_at.is_null());
}

}  // namespace ads
