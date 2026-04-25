/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/solana_message_address_table_lookup.h"

#include <string>
#include <vector>

#include "base/base64.h"
#include "base/strings/string_util.h"
#include "base/test/values_test_util.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/solana_address.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

TEST(SolanaMessageAddressTableLookupUnitTest, SerializeDeserialize) {
  SolanaMessageAddressTableLookup lookup(
      *SolanaAddress::FromBase58(
          "3Lu176FQzbQJCc8iL9PnmALbpMPhZeknoturApnXRDJw"),
      {1, 2}, {3, 4});

  std::vector<uint8_t> expected_bytes = {
      // base58-decoded of account_address
      34,  208, 53,  54, 75, 46,  112, 55, 123, 15,  232, 9,   45,
      178, 252, 196, 62, 64, 169, 213, 66, 87,  192, 16,  152, 108,
      254, 148, 183, 39, 51, 192, 2,   1,  2,   2,   3,   4};

  std::vector<uint8_t> bytes;
  lookup.Serialize(&bytes);
  EXPECT_EQ(bytes, expected_bytes);

  size_t bytes_index = 0;
  auto deserialized_lookup =
      SolanaMessageAddressTableLookup::Deserialize(bytes, &bytes_index);
  ASSERT_TRUE(deserialized_lookup);
  EXPECT_EQ(deserialized_lookup, lookup);
  EXPECT_EQ(bytes_index, bytes.size());

  bytes.clear();
  deserialized_lookup->Serialize(&bytes);
  EXPECT_EQ(bytes, expected_bytes);
}

TEST(SolanaMessageAddressTableLookupUnitTest, FromToValue) {
  std::string json(R"(
    {
      "account_key": "3Lu176FQzbQJCc8iL9PnmALbpMPhZeknoturApnXRDJw",
      "base64_encoded_write_indexes": "{base64_write_indexes}",
      "base64_encoded_read_indexes": "{base64_read_indexes}"
    }
  )");
  base::ReplaceFirstSubstringAfterOffset(
      &json, 0, "{base64_write_indexes}",
      base::Base64Encode(std::vector<uint8_t>({1, 2})));
  base::ReplaceFirstSubstringAfterOffset(
      &json, 0, "{base64_read_indexes}",
      base::Base64Encode(std::vector<uint8_t>({3, 4})));
  auto value = base::test::ParseJson(json);

  SolanaMessageAddressTableLookup lookup(
      *SolanaAddress::FromBase58(
          "3Lu176FQzbQJCc8iL9PnmALbpMPhZeknoturApnXRDJw"),
      {1, 2}, {3, 4});
  EXPECT_EQ(lookup,
            SolanaMessageAddressTableLookup::FromValue(value.GetDict()));
  EXPECT_EQ(value.GetDict(), lookup.ToValue());

  std::vector<std::string> invalid_value_strings = {
      "{}",
      R"({"account_key": "3Lu176FQzbQJCc8iL9PnmALbpMPhZeknoturApnXRDJw",
          "base64_encoded_write_indexes": "not base64",
          "base64_encoded_read_indexes": ""})",
      R"({"account_key": "3Lu176FQzbQJCc8iL9PnmALbpMPhZeknoturApnXRDJw",
          "base64_encoded_write_indexes": ""})",
      R"({"account_key": "3Lu176FQzbQJCc8iL9PnmALbpMPhZeknoturApnXRDJw",
          "base64_encoded_read_indexes": ""})",
      R"({"base64_encoded_write_indexes": "",
          "base64_encoded_read_indexes": ""})"};

  for (const auto& invalid_value_string : invalid_value_strings) {
    auto invalid_value = base::test::ParseJson(invalid_value_string);
    EXPECT_FALSE(
        SolanaMessageAddressTableLookup::FromValue(invalid_value.GetDict()))
        << ":" << invalid_value_string;
  }
}

TEST(SolanaMessageAddressTableLookupUnitTest, FromToMojomArray) {
  std::vector<SolanaMessageAddressTableLookup> lookups;
  lookups.emplace_back(*SolanaAddress::FromBase58(
                           "3Lu176FQzbQJCc8iL9PnmALbpMPhZeknoturApnXRDJw"),
                       std::vector<uint8_t>({1, 2}),
                       std::vector<uint8_t>({3, 4}));
  lookups.emplace_back(*SolanaAddress::FromBase58(
                           "3QpJ3j1vq1PfqJdvCcHKWuePykqoUYSvxyRb3Cnh79BD"),
                       std::vector<uint8_t>({5, 6}),
                       std::vector<uint8_t>({7, 8}));
  EXPECT_EQ(lookups,
            SolanaMessageAddressTableLookup::FromMojomArray(
                SolanaMessageAddressTableLookup::ToMojomArray(lookups)));

  // Test invalid account key from mojom array.
  std::vector<mojom::SolanaMessageAddressTableLookupPtr> mojom_lookups;
  mojom_lookups.emplace_back(mojom::SolanaMessageAddressTableLookup::New(
      "bad key", std::vector<uint8_t>(), std::vector<uint8_t>()));
  EXPECT_FALSE(SolanaMessageAddressTableLookup::FromMojomArray(mojom_lookups));
}

}  // namespace brave_wallet
