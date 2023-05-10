/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/legacy_migration/rewards/legacy_rewards_migration_payments_util.h"

#include <string>

#include "base/check.h"
#include "base/ranges/algorithm.h"
#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/time/time_util.h"

namespace brave_ads::rewards {

namespace {

std::string GetFormattedBalanceDate(const base::Time time) {
  base::Time::Exploded exploded;
  time.LocalExplode(&exploded);
  CHECK(exploded.HasValidValues());

  return base::StringPrintf("%04d-%02d", exploded.year, exploded.month);
}

absl::optional<PaymentInfo> GetPaymentForMonth(const PaymentList& payments,
                                               const base::Time time) {
  const std::string formatted_date = GetFormattedBalanceDate(time);

  const auto iter =
      base::ranges::find(payments, formatted_date, &PaymentInfo::month);
  if (iter == payments.cend()) {
    return absl::nullopt;
  }

  return *iter;
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

}  // namespace brave_ads::rewards
