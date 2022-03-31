/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/legacy_migration/rewards/legacy_rewards_migration_payments_util.h"

#include <algorithm>
#include <iterator>
#include <string>

#include "base/check.h"
#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "bat/ads/internal/legacy_migration/rewards/payment_info.h"
#include "bat/ads/internal/time_util.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ads {
namespace rewards {

namespace {

std::string GetFormattedBalanceDate(const base::Time time) {
  base::Time::Exploded exploded;
  time.LocalExplode(&exploded);
  DCHECK(exploded.HasValidValues());

  return base::StringPrintf("%04d-%02d", exploded.year, exploded.month);
}

absl::optional<PaymentInfo> GetPaymentForMonth(const PaymentList& payments,
                                               const base::Time time) {
  const std::string& formatted_date = GetFormattedBalanceDate(time);

  const auto iter = std::find_if(payments.cbegin(), payments.cend(),
                                 [&formatted_date](const PaymentInfo& payment) {
                                   return payment.month == formatted_date;
                                 });
  if (iter == payments.end()) {
    return absl::nullopt;
  }

  const PaymentInfo& payment = *iter;

  return payment;
}

}  // namespace

absl::optional<PaymentInfo> GetPaymentForThisMonth(
    const PaymentList& payments) {
  const base::Time time = GetLocalTimeAtBeginningOfThisMonth();
  return GetPaymentForMonth(payments, time);
}

absl::optional<PaymentInfo> GetPaymentForLastMonth(
    const PaymentList& payments) {
  const base::Time time = GetLocalTimeAtBeginningOfLastMonth();
  return GetPaymentForMonth(payments, time);
}

}  // namespace rewards
}  // namespace ads
