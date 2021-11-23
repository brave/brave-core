/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/brave_wallet_types.h"

#include <limits>

#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

TEST(BraveWalletTypesTest, OverflowWorksAsExpected) {
  // std::numberic_limits doesn't work right for uint256_t so construct the max
  // value ourselves.
  uint64_t max_val64 = std::numeric_limits<uint64_t>::max();
  uint256_t max_val256 = 0;
  for (size_t i = 0; i < 4; i++) {
    max_val256 <<= 64;
    max_val256 += static_cast<uint256_t>(max_val64);
  }
  ASSERT_EQ(max_val256 + static_cast<uint256_t>(1), static_cast<uint256_t>(0));
  uint256_t min_val = 0;
  ASSERT_EQ(min_val - static_cast<uint256_t>(1), max_val256);
}

}  // namespace brave_wallet
