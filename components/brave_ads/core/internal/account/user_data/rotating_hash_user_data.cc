/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/rotating_hash_user_data.h"

#include <cstdint>
#include <string>

#include "base/base64.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/common/interfaces/ads.mojom.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"
#include "brave/components/brave_ads/core/internal/common/crypto/crypto_util.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state.h"

namespace brave_ads::user_data {

namespace {
constexpr char kRotatingHashKey[] = "rotating_hash";
}  // namespace

base::Value::Dict GetRotatingHash(const TransactionInfo& transaction) {
  base::Value::Dict user_data;

  const auto& sys_info = GlobalState::GetInstance()->SysInfo();
  const std::string& device_id = sys_info.device_id;
  if (device_id.empty()) {
    return user_data;
  }

  const int64_t timestamp_rounded_to_nearest_hour = static_cast<int64_t>(
      base::Time::Now().ToDoubleT() / base::Time::kSecondsPerHour);
  const std::string timestamp =
      base::NumberToString(timestamp_rounded_to_nearest_hour);

  const std::string rotating_hash = base::Base64Encode(crypto::Sha256(
      base::StrCat({device_id, transaction.creative_instance_id, timestamp})));
  user_data.Set(kRotatingHashKey, rotating_hash);

  return user_data;
}

}  // namespace brave_ads::user_data
