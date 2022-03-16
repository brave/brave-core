/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/brave_wallet_types.h"

#include <limits>

#include "base/json/json_reader.h"
#include "base/values.h"
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

TEST(BraveWalletTypesTest, SolanaSignatureStatusFromValue) {
  absl::optional<base::Value> value = base::JSONReader::Read(R"({
    "slot": "18446744073709551615",
    "confirmations": "10",
    "err": "",
    "confirmation_status": "confirmed"
  })");
  ASSERT_TRUE(value);

  absl::optional<SolanaSignatureStatus> status =
      SolanaSignatureStatus::FromValue(*value);
  ASSERT_TRUE(status);
  EXPECT_EQ(std::numeric_limits<uint64_t>::max(), status->slot);
  EXPECT_EQ(10u, status->confirmations);
  EXPECT_EQ("", status->err);
  EXPECT_EQ("confirmed", status->confirmation_status);

  std::vector<std::string> invalid_value_strings = {
      "{}", "[]",
      // slot/confirmations > uint64_t max
      R"({"slot": "18446744073709551616", "confirmations": "10",
          "err": "", "confirmation_status": "confirmed"})",
      R"({"slot": "72", "confirmations": "18446744073709551616",
          "err": "", "confirmation_status": "confirmed"})",
      // Missing fields
      R"({"confirmations": "10", "err": "",
          "confirmation_status": "confirmed"})",
      R"({"slot": "72", "err": "", "confirmation_status": "confirmed"})",
      R"({"slot": "72", "confirmations": "10",
          "confirmation_status": "confirmed"})",
      R"({"slot": "72", "confirmations": "10", "err": ""})"};

  for (const auto& invalid_value_string : invalid_value_strings) {
    absl::optional<base::Value> invalid_value =
        base::JSONReader::Read(invalid_value_string);
    ASSERT_TRUE(invalid_value) << ":" << invalid_value_string;
    EXPECT_FALSE(SolanaSignatureStatus::FromValue(*invalid_value))
        << ":" << invalid_value_string;
  }
}

TEST(BraveWalletTypesTest, SolanaSignatureStatusToValue) {
  SolanaSignatureStatus status;
  status.slot = std::numeric_limits<uint64_t>::max();
  status.confirmations = 10;
  status.err = "";
  status.confirmation_status = "confirmed";

  base::Value value = status.ToValue();
  ASSERT_TRUE(value.is_dict());
  EXPECT_EQ(*value.FindStringKey("slot"), "18446744073709551615");
  EXPECT_EQ(*value.FindStringKey("confirmations"), "10");
  EXPECT_EQ(*value.FindStringKey("err"), status.err);
  EXPECT_EQ(*value.FindStringKey("confirmation_status"),
            status.confirmation_status);

  auto status_from_value = SolanaSignatureStatus::FromValue(value);
  ASSERT_TRUE(status_from_value);
  EXPECT_EQ(*status_from_value, status);
}

}  // namespace brave_wallet
