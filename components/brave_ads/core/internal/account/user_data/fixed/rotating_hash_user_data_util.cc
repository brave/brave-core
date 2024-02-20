/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/fixed/rotating_hash_user_data_util.h"

#include <cstdint>
#include <vector>

#include "base/base64.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"
#include "brave/components/brave_ads/core/internal/common/crypto/crypto_util.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state.h"

namespace brave_ads {

std::optional<std::string> BuildRotatingHash(
    const TransactionInfo& transaction) {
  const std::string& device_id =
      GlobalState::GetInstance()->SysInfo().device_id;
  if (device_id.empty()) {
    return std::nullopt;
  }

  const std::string hours = base::NumberToString(
      base::Time::Now().ToDeltaSinceWindowsEpoch().InHours());

  const std::vector<uint8_t> rotating_hash = crypto::Sha256(
      base::StrCat({device_id, transaction.creative_instance_id, hours}));

  return base::Base64Encode(rotating_hash);
}

}  // namespace brave_ads
