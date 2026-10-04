/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_test_utils.h"

#include <string_view>
#include <vector>

#include "base/containers/span.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace {

std::vector<uint8_t> BytesForTest(std::string_view value) {
  auto bytes = base::as_byte_span(value);
  return std::vector(bytes.begin(), bytes.end());
}

}  // namespace

TEST(PolkadotTestUtilsUnitTest,
     ReplaceNthOccurrenceReplacesRequestedOccurrence) {
  auto bytes = BytesForTest("transfer transfer transfer");

  EXPECT_TRUE(ReplaceNthOccurrence(bytes, "transfer", "replaced",
                                   /*occurrence=*/1));

  EXPECT_EQ(bytes, BytesForTest("transfer replaced transfer"));
}

TEST(PolkadotTestUtilsUnitTest,
     ReplaceNthOccurrenceSupportsDifferentLengthReplacement) {
  auto bytes = BytesForTest("before transfer after");

  EXPECT_TRUE(ReplaceNthOccurrence(bytes, "transfer", "x",
                                   /*occurrence=*/0));

  EXPECT_EQ(bytes, BytesForTest("before x after"));
}

TEST(PolkadotTestUtilsUnitTest, ReplaceNthOccurrenceMissingNeedleFails) {
  auto bytes = BytesForTest("before transfer after");
  const auto original = bytes;

  EXPECT_FALSE(ReplaceNthOccurrence(bytes, "missing", "x",
                                    /*occurrence=*/0));

  EXPECT_EQ(bytes, original);
}

TEST(PolkadotTestUtilsUnitTest, ReplaceNthOccurrenceOutOfRangeFails) {
  auto bytes = BytesForTest("transfer transfer");
  const auto original = bytes;

  EXPECT_FALSE(ReplaceNthOccurrence(bytes, "transfer", "replaced",
                                    /*occurrence=*/2));

  EXPECT_EQ(bytes, original);
}

}  // namespace brave_wallet
