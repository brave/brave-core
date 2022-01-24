/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_gas_utils.h"

#include <cmath>

#include "base/check.h"
#include "base/logging.h"
#include "brave/components/brave_wallet/common/hex_utils.h"

namespace brave_wallet {

namespace eth {

// Assumes there are 3 reward percentiles per element
bool GetSuggested1559Fees(const std::vector<std::string>& base_fee_per_gas,
                          const std::vector<double>& gas_used_ratio,
                          const std::string& oldest_block,
                          const std::vector<std::vector<std::string>>& reward,
                          uint256_t* low_priority_fee,
                          uint256_t* avg_priority_fee,
                          uint256_t* high_priority_fee,
                          uint256_t* suggested_base_fee_per_gas) {
  CHECK(low_priority_fee);
  CHECK(avg_priority_fee);
  CHECK(high_priority_fee);
  CHECK(suggested_base_fee_per_gas);

  // Not enough info to determine values
  if (base_fee_per_gas.empty())
    return false;

  // "pending" is the last element in base_fee_per_gas
  // "latest" is the 2nd last element in base_fee_per_gas
  std::string pending_base_fee_per_gas = base_fee_per_gas.back();
  uint256_t pending_base_fee_per_gas_uint;
  if (!HexValueToUint256(pending_base_fee_per_gas,
                         &pending_base_fee_per_gas_uint))
    return false;

  // We use "double" math and this is unlikely to get hit, so return false
  // if the value is too big.
  if (pending_base_fee_per_gas_uint > kMaxSafeIntegerUint64)
    return false;

  // Compiler crashes without these 2 casts :(
  double pending_base_fee_per_gas_dbl =
      (double)(uint64_t)pending_base_fee_per_gas_uint;
  // The base fee is not part of the RLP. We only specify priority fee and max
  // fee. So the excess will be refunded and it will be at most 12.5% It's best
  // to assume a larger value here so there's a better chance it will get in the
  // next block and if it's too high it will go back to the user
  pending_base_fee_per_gas_dbl *= 1.125;
  pending_base_fee_per_gas_dbl = std::ceil(pending_base_fee_per_gas_dbl);
  // Compiler crashes without these 2 casts :(
  *suggested_base_fee_per_gas =
      (uint256_t)(uint64_t)pending_base_fee_per_gas_dbl;

  uint256_t* output_fee[3] = {low_priority_fee, avg_priority_fee,
                              high_priority_fee};
  const uint256_t fallback_priority_fee = 2e9;
  for (size_t i = 0; i < 3; i++) {
    uint256_t& current_output_fee = *(output_fee[i]);
    current_output_fee = 0;
    if (reward.size() == 0) {
      current_output_fee = fallback_priority_fee;
      continue;
    }
    for (const std::vector<std::string>& v : reward) {
      // Unexpected reward size
      if (v.size() != 3)
        return false;

      uint256_t current_reward_value;
      if (!HexValueToUint256(v[i], &current_reward_value))
        return false;
      current_output_fee += current_reward_value;
    }
    if (current_output_fee > kMaxSafeIntegerUint64)
      return false;
    // Compiler crashes without these 2 casts :(
    double output_fee_dbl = (double)(uint64_t)current_output_fee;
    // The excess will be refunded and it will be at most 12.5%
    output_fee_dbl /= reward.size();
    output_fee_dbl = std::ceil(output_fee_dbl);
    // Compiler crashes without these 2 casts :(
    current_output_fee = (uint256_t)(uint64_t)output_fee_dbl;
  }
  return true;
}

}  // namespace eth

}  // namespace brave_wallet
