/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_DEPOSITS_DEPOSITS_DATABASE_TABLE_ALIASES_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_DEPOSITS_DEPOSITS_DATABASE_TABLE_ALIASES_H_

#include <functional>

#include "bat/ads/internal/account/deposits/deposit_info.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ads {

using GetDepositsCallback =
    std::function<void(const bool success,
                       const absl::optional<DepositInfo>& deposit_optional)>;

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_DEPOSITS_DEPOSITS_DATABASE_TABLE_ALIASES_H_
