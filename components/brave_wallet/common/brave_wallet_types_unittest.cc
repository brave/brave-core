/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/brave_wallet_types.h"

#include <limits>
#include <optional>

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
  std::optional<base::Value> value = base::JSONReader::Read(R"({
    "slot": "18446744073709551615",
    "confirmations": "10",
    "err": "",
    "confirmation_status": "confirmed"
  })");
  ASSERT_TRUE(value);

  std::optional<SolanaSignatureStatus> status =
      SolanaSignatureStatus::FromValue(value->GetDict());
  ASSERT_TRUE(status);
  EXPECT_EQ(std::numeric_limits<uint64_t>::max(), status->slot);
  EXPECT_EQ(10u, status->confirmations);
  EXPECT_EQ("", status->err);
  EXPECT_EQ("confirmed", status->confirmation_status);

  std::vector<std::string> invalid_value_strings = {
      "{}",
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
    std::optional<base::Value> invalid_value =
        base::JSONReader::Read(invalid_value_string);
    ASSERT_TRUE(invalid_value) << ":" << invalid_value_string;
    EXPECT_FALSE(SolanaSignatureStatus::FromValue(invalid_value->GetDict()))
        << ":" << invalid_value_string;
  }
}

TEST(BraveWalletTypesTest, SolanaSignatureStatusToValue) {
  SolanaSignatureStatus status;
  status.slot = std::numeric_limits<uint64_t>::max();
  status.confirmations = 10;
  status.err = "";
  status.confirmation_status = "confirmed";

  base::Value::Dict value = status.ToValue();
  EXPECT_EQ(*value.FindString("slot"), "18446744073709551615");
  EXPECT_EQ(*value.FindString("confirmations"), "10");
  EXPECT_EQ(*value.FindString("err"), status.err);
  EXPECT_EQ(*value.FindString("confirmation_status"),
            status.confirmation_status);

  auto status_from_value = SolanaSignatureStatus::FromValue(value);
  ASSERT_TRUE(status_from_value);
  EXPECT_EQ(*status_from_value, status);
}

TEST(BraveWalletTypesTest, ValidSolidityBits) {
  for (size_t i = 8; i <= 256; i += 8) {
    EXPECT_TRUE(ValidSolidityBits(i));
  }
  std::vector<size_t> invalid_num_bits = {0, 7, 257};
  for (size_t i : invalid_num_bits) {
    EXPECT_FALSE(ValidSolidityBits(i));
    EXPECT_EQ(MaxSolidityUint(i), std::nullopt);
    EXPECT_EQ(MaxSolidityInt(i), std::nullopt);
    EXPECT_EQ(MinSolidityInt(i), std::nullopt);
  }
}

TEST(BraveWalletTypesTest, MaxSolidityUint) {
  EXPECT_EQ(MaxSolidityUint(8), uint256_t(255));
  EXPECT_EQ(MaxSolidityUint(16), uint256_t(65535));
  EXPECT_EQ(MaxSolidityUint(24), uint256_t(16777215));
  EXPECT_EQ(MaxSolidityUint(128),
            uint256_t(std::numeric_limits<uint128_t>::max()));
  EXPECT_EQ(MaxSolidityUint(256),
            uint256_t(std::numeric_limits<uint256_t>::max()));
}

TEST(BraveWalletTypesTest, MaxSolidityInt) {
  EXPECT_EQ(MaxSolidityInt(8), int256_t(127));
  EXPECT_EQ(MaxSolidityInt(16), int256_t(32767));
  EXPECT_EQ(MaxSolidityInt(24), int256_t(8388607));
  EXPECT_EQ(MaxSolidityInt(128), int256_t(kMax128BitInt));
  EXPECT_EQ(MaxSolidityInt(256), int256_t(kMax256BitInt));
}

TEST(BraveWalletTypesTest, MinSolidityInt) {
  EXPECT_EQ(MinSolidityInt(8), int256_t(-128));
  EXPECT_EQ(MinSolidityInt(16), int256_t(-32768));
  EXPECT_EQ(MinSolidityInt(24), int256_t(-8388608));
  EXPECT_EQ(MinSolidityInt(128), int256_t(kMin128BitInt));
  EXPECT_EQ(MinSolidityInt(256), int256_t(kMin256BitInt));
}

}  // namespace brave_wallet
