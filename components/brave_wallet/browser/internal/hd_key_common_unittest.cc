/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/internal/hd_key_common.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

TEST(DerivationIndex, Normal) {
  for (auto test_case : std::to_array({0u, 1u, kHardenedOffset - 1})) {
    auto index = DerivationIndex::Normal(test_case);
    EXPECT_TRUE(index.IsValid());
    EXPECT_FALSE(index.is_hardened());
    EXPECT_EQ(index.GetValue(), test_case);
  }

  for (auto test_case : std::to_array(
           {kHardenedOffset, kHardenedOffset - 1 + kHardenedOffset})) {
    auto index = DerivationIndex::Normal(test_case);
    EXPECT_FALSE(index.IsValid());
    EXPECT_FALSE(index.GetValue());
  }
}

TEST(DerivationIndex, Hardened) {
  for (auto test_case : std::to_array({0u, 1u, kHardenedOffset - 1})) {
    auto index = DerivationIndex::Hardened(test_case);
    EXPECT_TRUE(index.IsValid());
    EXPECT_TRUE(index.is_hardened());
    EXPECT_EQ(index.GetValue(), kHardenedOffset + test_case);
  }

  for (auto test_case : std::to_array(
           {kHardenedOffset, kHardenedOffset - 1 + kHardenedOffset})) {
    auto index = DerivationIndex::Hardened(test_case);
    EXPECT_FALSE(index.IsValid());
    EXPECT_FALSE(index.GetValue());
  }
}

}  // namespace brave_wallet
