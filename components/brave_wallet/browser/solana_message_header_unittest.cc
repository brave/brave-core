/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/solana_message_header.h"

#include <string>
#include <vector>

#include "base/test/values_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

TEST(SolanaMessageHeaderUnitTest, FromToValue) {
  auto dict = base::test::ParseJsonDict(R"({
      "num_required_signatures": "255",
      "num_readonly_signed_accounts": "0",
      "num_readonly_unsigned_accounts": "1"
  })");
  SolanaMessageHeader header(255, 0, 1);
  EXPECT_EQ(header, SolanaMessageHeader::FromValue(dict));
  EXPECT_EQ(dict, header.ToValue());

  std::vector<std::string> invalid_value_strings = {
      "{}",
      R"({"num_required_signatures": "1",
          "num_readonly_signed_accounts": "0"})",
      R"({"num_required_signatures": "1",
          "num_readonly_unsigned_accounts": "0"})",
      R"({"num_readonly_signed_accounts": "1",
          "num_readonly_signed_accounts": "0"})",
      R"({"num_required_signatures": "1",
          "num_readonly_signed_accounts": "256",
          "num_readonly_unsigned_accounts": "0"})",
      R"({"num_required_signatures": "1",
          "num_readonly_signed_accounts": "255",
          "num_readonly_unsigned_accounts": "HELLO"})",
      R"({"num_required_signatures": "-1",
          "num_readonly_signed_accounts": "255",
          "num_readonly_unsigned_accounts": "0"})"};

  for (const auto& invalid_value_string : invalid_value_strings) {
    auto invalid_dict = base::test::ParseJsonDict(invalid_value_string);
    EXPECT_FALSE(SolanaMessageHeader::FromValue(invalid_dict))
        << ":" << invalid_value_string;
  }
}

}  // namespace brave_wallet
