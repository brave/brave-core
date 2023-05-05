/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"

#include <limits>

#include "base/numerics/ranges.h"

namespace brave_ads {

TransactionInfo::TransactionInfo() = default;

TransactionInfo::TransactionInfo(const TransactionInfo& other) = default;

TransactionInfo& TransactionInfo::operator=(const TransactionInfo& other) =
    default;

TransactionInfo::TransactionInfo(TransactionInfo&& other) noexcept = default;

TransactionInfo& TransactionInfo::operator=(TransactionInfo&& other) noexcept =
    default;

TransactionInfo::~TransactionInfo() = default;

// TODO(https://github.com/brave/brave-browser/issues/23087):
// |base::IsApproximatelyEqual| can be removed for timestamp comparisons once
// timestamps are persisted using |ToDeltaSinceWindowsEpoch| and
// |FromDeltaSinceWindowsEpoch| in microseconds.
bool TransactionInfo::operator==(const TransactionInfo& other) const {
  return id == other.id &&
         base::IsApproximatelyEqual(created_at.ToDoubleT(),
                                    other.created_at.ToDoubleT(),
                                    std::numeric_limits<double>::epsilon()) &&
         creative_instance_id == other.creative_instance_id &&
         segment == other.segment &&
         base::IsApproximatelyEqual(value, other.value,
                                    std::numeric_limits<double>::epsilon()) &&
         ad_type == other.ad_type &&
         confirmation_type == other.confirmation_type &&
         base::IsApproximatelyEqual(reconciled_at.ToDoubleT(),
                                    other.reconciled_at.ToDoubleT(),
                                    std::numeric_limits<double>::epsilon());
}

bool TransactionInfo::operator!=(const TransactionInfo& other) const {
  return !(*this == other);
}

bool TransactionInfo::IsValid() const {
  return !id.empty() && !creative_instance_id.empty() &&
         ad_type != AdType::kUndefined &&
         confirmation_type != ConfirmationType::kUndefined &&
         !created_at.is_null();
}

}  // namespace brave_ads
