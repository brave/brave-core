/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/fixed/rotating_hash_user_data.h"

#include <optional>
#include <string>
#include <string_view>

#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"
#include "brave/components/brave_ads/core/internal/account/user_data/fixed/rotating_hash_user_data_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"

namespace brave_ads {

namespace {
constexpr std::string_view kRotatingHashKey = "rotatingHash";
}  // namespace

base::Value::Dict BuildRotatingHashUserData(
    const TransactionInfo& transaction) {
  if (!UserHasJoinedBraveRewards()) {
    return {};
  }

  std::optional<std::string> rotating_hash = BuildRotatingHash(transaction);
  if (!rotating_hash) {
    // Invalid rotating hash.
    return {};
  }

  return base::Value::Dict().Set(kRotatingHashKey, *rotating_hash);
}

}  // namespace brave_ads
