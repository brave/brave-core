/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <utility>
#include <vector>

#include "brave/components/brave_wallet/browser/eth_gas_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace eth {

TEST(EthGasUtilsTest, GetSuggested1559Fees) {
  const std::vector<std::string> base_fee_per_gas{
      "0x257093e880", "0x20f4138789", "0x20b04643ea",
      "0x1da8692acc", "0x215d00b8c8", "0x24beaded75"};
  const std::vector<double> gas_used_ratio{
      0.020687709938714324, 0.4678514936136911, 0.12914042746424212, 0.999758,
      0.9054214892490816};
  const std::vector<std::vector<std::string>> reward{
      {"0x77359400", "0x77359400", "0x3a3eb2ac0"},
      {"0x59682f00", "0x77359400", "0x48ae2f980"},
      {"0x59682f00", "0x9502f900", "0x17d1ffc7d6"},
      {"0xee6b2800", "0x32bd81734", "0xda2b71b34"},
      {"0x77359400", "0x77359400", "0x2816a6cfb"}};
  const std::string oldest_block = "0xd6b1b0";

  uint256_t low_priority_fee;
  uint256_t avg_priority_fee;
  uint256_t high_priority_fee;
  uint256_t suggested_base_fee_per_gas;
  EXPECT_TRUE(GetSuggested1559Fees(
      base_fee_per_gas, gas_used_ratio, oldest_block, reward, &low_priority_fee,
      &avg_priority_fee, &high_priority_fee, &suggested_base_fee_per_gas));
  EXPECT_EQ(suggested_base_fee_per_gas, 177545128740ULL);
  // (2000000000 + 1500000000 + 1500000000 + 4000000000 + 2000000000) / 5
  EXPECT_EQ(low_priority_fee, 2200000000ULL);
  // (2000000000 + 2000000000 + 2500000000 + 13620483892 + 2000000000) / 5
  EXPECT_EQ(avg_priority_fee, 4424096779ULL);
  // (15635000000 + 19510000000 + 102307448790 + 58564483892 + 10761170171) / 5
  EXPECT_EQ(high_priority_fee, 41355620571ULL);

  // No reward info should return default for priority fees
  EXPECT_TRUE(GetSuggested1559Fees(
      base_fee_per_gas, gas_used_ratio, oldest_block,
      std::vector<std::vector<std::string>>(), &low_priority_fee,
      &avg_priority_fee, &high_priority_fee, &suggested_base_fee_per_gas));
  EXPECT_EQ(suggested_base_fee_per_gas, 177545128740ULL);
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

}  // namespace eth

}  // namespace brave_wallet
