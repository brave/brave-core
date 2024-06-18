/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <utility>
#include <vector>

#include "brave/components/brave_wallet/browser/eth_gas_utils.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet::eth {

TEST(EthGasUtilsTest, ScaleBaseFeePerGas) {
  // OK: scale 0x64 (100)
  EXPECT_EQ(ScaleBaseFeePerGas("0x64"), 133ULL);

  // OK: scale 0x0 (0)
  EXPECT_EQ(ScaleBaseFeePerGas("0x0"), 0ULL);

  // KO: invalid hex
  EXPECT_FALSE(ScaleBaseFeePerGas("invalid"));

  // KO: value is too big
  EXPECT_FALSE(
      ScaleBaseFeePerGas(Uint256ValueToHex(kMaxSafeIntegerUint64 + 1)));
}

TEST(EthGasUtilsTest, GetSuggested1559Fees) {
  const std::vector<std::string> base_fee_per_gas{
      "0x257093e880", "0x20f4138789", "0x20b04643ea",
      "0x1da8692acc", "0x215d00b8c8", "0x24beaded75"};
  const std::vector<double> gas_used_ratio{
      0.020687709938714324, 0.4678514936136911, 0.12914042746424212, 0.999758,
      0.9054214892490816};
  const std::vector<std::vector<std::string>> reward{
      {"0x9502f900" /* 2.5gwei */, "0x9502f900" /* 2.5gwei */,
       "0xB2D05E00" /* 3gwei */},
      {"0x9502f900" /* 2.5gwei */, "0xB2D05E00" /* 2.5gwei */,
       "0xB2D05E00" /* 3gwei */},
      {"0x9502f900" /* 2.5gwei */, "0x9502f900" /* 2.5gwei */,
       "0xB2D05E00" /* 3gwei */},
      {"0x77359400" /* 2 gwei */, "0x77359400" /* 2.5gwei */,
       "0x9502f900" /* 2.5gwei */},
      {"0x77359400" /* 2 gwei */, "0x77359400" /* 2.5gwei */,
       "0x9502f900" /* 2.5gwei */}};
  const std::string oldest_block = "0xd6b1b0";

  uint256_t low_priority_fee;
  uint256_t avg_priority_fee;
  uint256_t high_priority_fee;
  uint256_t suggested_base_fee_per_gas;
  EXPECT_TRUE(GetSuggested1559Fees(
      base_fee_per_gas, gas_used_ratio, oldest_block, reward, &low_priority_fee,
      &avg_priority_fee, &high_priority_fee, &suggested_base_fee_per_gas));
  EXPECT_EQ(suggested_base_fee_per_gas, 209897796643ULL);
  EXPECT_EQ(low_priority_fee, 2000000000ULL);
  EXPECT_EQ(avg_priority_fee, 2500000000ULL);
  EXPECT_EQ(high_priority_fee, 3000000000ULL);

  // No reward info should return default for priority fees
  EXPECT_TRUE(GetSuggested1559Fees(
      base_fee_per_gas, gas_used_ratio, oldest_block,
      std::vector<std::vector<std::string>>(), &low_priority_fee,
      &avg_priority_fee, &high_priority_fee, &suggested_base_fee_per_gas));
  EXPECT_EQ(suggested_base_fee_per_gas, 209897796643ULL);
  EXPECT_EQ(low_priority_fee, 2000000000ULL);
  EXPECT_EQ(avg_priority_fee, 2000000000ULL);
  EXPECT_EQ(high_priority_fee, 2000000000ULL);

  // No base fee per gas info should return false
  EXPECT_FALSE(GetSuggested1559Fees(std::vector<std::string>(), gas_used_ratio,
                                    oldest_block, reward, &low_priority_fee,
                                    &avg_priority_fee, &high_priority_fee,
                                    &suggested_base_fee_per_gas));

  // Invalid reward size returns false
  EXPECT_FALSE(GetSuggested1559Fees(
      base_fee_per_gas, gas_used_ratio, oldest_block,
      (std::vector<std::vector<std::string>>{{"0x77359400", "0x77359400"}}),
      &low_priority_fee, &avg_priority_fee, &high_priority_fee,
      &suggested_base_fee_per_gas));
}

}  // namespace brave_wallet::eth
