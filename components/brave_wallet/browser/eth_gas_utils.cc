/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_gas_utils.h"

#include <algorithm>
#include <cmath>

#include "base/check.h"
#include "base/logging.h"
#include "brave/components/brave_wallet/common/hex_utils.h"

namespace brave_wallet {

namespace eth {

// Assumes there are 3 reward percentiles per element
// The first for low, the second for avg, and the third for high.
// The following calculations will be made:
// - suggested_base_fee_per_gas will be the pending
//   base_fee_per_gas (last element) * 12.5%
// - avg_priority_fee will be the 0.4 * length's element of the sorted
//   reward array.
// - The same applies to low_prirority_fee, but if it was equal to avg
//   then we walk it back to the next smallest element if possible.
// - The same applies to high_prirority_fee, but if it was equal to avg
//   then we walk it forward to the next biggest element if possible.
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
      static_cast<double>(static_cast<uint64_t>(pending_base_fee_per_gas_uint));
  // The base fee is not part of the RLP. We only specify priority fee and max
  // fee. So the excess will be refunded and it will be at most 12.5% It's best
  // to assume a larger value here so there's a better chance it will get in the
  // next block and if it's too high it will go back to the user
  pending_base_fee_per_gas_dbl *= 1.125;
  pending_base_fee_per_gas_dbl = std::floor(pending_base_fee_per_gas_dbl);
  // Compiler crashes without these 2 casts :(
  *suggested_base_fee_per_gas =
      (uint256_t)(uint64_t)pending_base_fee_per_gas_dbl;

  const uint256_t fallback_priority_fee = uint256_t(2e9);
  *low_priority_fee = fallback_priority_fee;
  *avg_priority_fee = fallback_priority_fee;
  *high_priority_fee = fallback_priority_fee;

  // Leave it at fallback values if no reward info is passed
  if (reward.size() == 0)
    return true;

  std::vector<uint256_t> priority_fee_uints[3];
  uint256_t* priority_fees[3] = {low_priority_fee, avg_priority_fee,
                                 high_priority_fee};
  for (size_t i = 0; i < 3; i++) {
    uint256_t& current_priority_fee = *(priority_fees[i]);
    std::vector<uint256_t>& current_priority_fee_uints = priority_fee_uints[i];
    bool invalid_data = false;
    // Convert the string priority fees to uints
    std::transform(reward.begin(), reward.end(),
                   std::back_inserter(current_priority_fee_uints),
                   [&](const std::vector<std::string>& v) -> uint256_t {
                     uint256_t val = fallback_priority_fee;
                     if (v.size() != 3) {
                       invalid_data = true;
                     } else if (!HexValueToUint256(v[i], &val)) {
                       invalid_data = true;
                     }
                     return val;
                   });

    // We allow no reward info but we don't allow invalid reward info
    if (invalid_data) {
      return false;
    }

    // Sort the priroirty fee uints
    std::sort(current_priority_fee_uints.begin(),
              current_priority_fee_uints.end());
    // Calculate the avg priorty fee first to be the 40th percentile of the avg
    // percentiles.  We use this same method as the initial value for low
    // and high too.
    size_t percentile_index = current_priority_fee_uints.size() * 0.4;

    current_priority_fee = current_priority_fee_uints[percentile_index];
  }

  // Re-adjust the percentiles for low down to the next non-equal value if
  // possible
  size_t index = priority_fee_uints[0].size() * 0.4;
  while (index >= 1 && *low_priority_fee == *avg_priority_fee) {
    index--;
    *low_priority_fee = priority_fee_uints[0][index];
  }

  // Re-adjust the percentiles for high up to the next non-equal value if
  // possible
  index = priority_fee_uints[2].size() * 0.4;
  while (index < priority_fee_uints[2].size() - 1 &&
         *high_priority_fee == *avg_priority_fee) {
    index++;
    *high_priority_fee = priority_fee_uints[2][index];
  }
  return true;
}

}  // namespace eth

}  // namespace brave_wallet
