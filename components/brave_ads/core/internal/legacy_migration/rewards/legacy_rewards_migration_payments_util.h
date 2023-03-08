/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_LEGACY_MIGRATION_REWARDS_LEGACY_REWARDS_MIGRATION_PAYMENTS_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_LEGACY_MIGRATION_REWARDS_LEGACY_REWARDS_MIGRATION_PAYMENTS_UTIL_H_

#include "brave/components/brave_ads/core/internal/legacy_migration/rewards/payment_info.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ads::rewards {

absl::optional<PaymentInfo> GetPaymentForThisMonth(const PaymentList& payments);
absl::optional<PaymentInfo> GetPaymentForLastMonth(const PaymentList& payments);

}  // namespace ads::rewards

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_LEGACY_MIGRATION_REWARDS_LEGACY_REWARDS_MIGRATION_PAYMENTS_UTIL_H_
