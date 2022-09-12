/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_LEGACY_MIGRATION_REWARDS_LEGACY_REWARDS_MIGRATION_PAYMENTS_JSON_READER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_LEGACY_MIGRATION_REWARDS_LEGACY_REWARDS_MIGRATION_PAYMENTS_JSON_READER_H_

#include <string>

#include "absl/types/optional.h"
#include "bat/ads/internal/legacy_migration/rewards/payment_info.h"

namespace ads::rewards::json::reader {

absl::optional<PaymentList> ReadPayments(const std::string& json);

}  // namespace ads::rewards::json::reader

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_LEGACY_MIGRATION_REWARDS_LEGACY_REWARDS_MIGRATION_PAYMENTS_JSON_READER_H_
