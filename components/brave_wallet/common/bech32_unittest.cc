
/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/bech32.h"

#include <array>
#include <optional>
#include <string_view>
#include <utility>

#include "base/strings/string_number_conversions.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet::bech32 {

// https://github.com/bitcoin/bips/blob/master/bip-0173.mediawiki#test-vectors
TEST(Bech32UnitTest, TestVectors_BIP173) {
  auto valid_bech32_cases = std::to_array<std::string_view>({
      "A12UEL5L",
      "a12uel5l",
      "an83characterlonghumanreadablepartthatcontainsthenumber1andtheexcludedc"
      "haractersbio1tt5tgs",
      "abcdef1qpzry9x8gf2tvdw0s3jn54khce6mua7lmqqqxw",
      "11qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq"
      "qqqqqqqqqqqqqc8247j",
      "split1checkupstagehandshakeupstreamerranterredcaperred2y9e3w",
      "?1ezyfcl",
  });

  for (auto address : valid_bech32_cases) {
    SCOPED_TRACE(address);
    EXPECT_TRUE(Decode(address));
  }

  auto invalid_bech32_cases = std::to_array<std::string_view>({
      "\x{20}1nwldj5",
      "\x{7F}1axkwrx",
      "\x{80}1eym55h",
      "an84characterslonghumanreadablepartthatcontainsthenumber1andtheexcludedc"
      "haractersbio1569pvx",
      "pzry9x0s0muk",
      "1pzry9x0s0muk",
      "x1b4n0q5v",
      "li1dgmt3",
      "de1lg7wt",
      "A1G7SGD8",
      "10a06t8",
      "1qzzfhee",
  });

  for (auto address : invalid_bech32_cases) {
    SCOPED_TRACE(address);
    EXPECT_FALSE(Decode(address));
  }

  const auto segwit_conversion_cases = std::to_array<
      std::pair<std::string_view, std::string_view>>({
      {"bc1qw508d6qejxtdg4y5r3zarvary0c5xw7kv8f3t4",
       "0014"
       "751e76e8199196d454941c45d1b3a323f1433bd6"},
      {
          "tb1qrp33g0q5c5txsp9arysrx4k6zdkfs4nce4xj0gdcccefvpysxf3q0sl5k7",
          "0020"
          "1863143c14c5166804bd19203356da136c985678cd4d27a1b8c6329604903262",
      },
      {
          "bc1pw508d6qejxtdg4y5r3zarvary0c5xw7kw508d6qejxtdg4y5r3zarvary0c5xw7k"
          "t5nd6y",
          "5128"
          "751e76e8199196d454941c45d1b3a323f1433bd6751e76e8199196d454941c45d1b3"
          "a323f1433bd6",
      },
      {
          "bc1sw50qgdz25j",
          "6002"
          "751e",
      },
      {
          "bc1zw508d6qejxtdg4y5r3zarvaryvaxxpcs",
          "5210"
          "751e76e8199196d454941c45d1b3a323",
      },
      {
          "tb1qqqqqp399et2xygdj5xreqhjjvcmzhxw4aywxecjdzew6hylgvsesrxh6hy",
          "0020"
          "000000c4a5cad46221b2a187905e5266362b99d5e91c6ce24d165dab93e86433",
      },
      {
          "tb1pqqqqp399et2xygdj5xreqhjjvcmzhxw4aywxecjdzew6hylgvsesf3hn0c",
          "5120"
          "000000c4a5cad46221b2a187905e5266362b99d5e91c6ce24d165dab93e86433",
      },
      {
          "bc1p0xlxvlhemja6c4dqv22uapctqupfhlxm9h8z3k2e72q4k9hcz7vqzk5jj0",
          "5120"
          "79be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798",
      },
  });

  for (auto& test_case : segwit_conversion_cases) {
    auto address = test_case.first;
    SCOPED_TRACE(address);

    // Cut off 2 bytes of opcodes from `script_pub_key`.
    auto bytes_hex = test_case.second.substr(4);

    // Test that we can encode/decode it as a bitcoin bech32 address.
    auto decoded = DecodeForBitcoin(address);
    ASSERT_TRUE(decoded);
    EXPECT_EQ(base::HexEncodeLower(decoded->data), bytes_hex);
    EXPECT_EQ(EncodeForBitcoin(decoded->data, decoded->hrp, decoded->witness),
              address);
  }
}

// https://github.com/bitcoin/bips/blob/master/bip-0350.mediawiki#test-vectors
TEST(Bech32UnitTest, TestVectors_BIP350) {
  auto valid_bech32_cases = std::to_array<std::string_view>({
      "A1LQFN3A",
      "a1lqfn3a",
      "an83characterlonghumanreadablepartthatcontainsthetheexcludedcharactersbi"
      "oandnumber11sg7hg6",
      "abcdef1l7aum6echk45nj3s0wdvt2fg8x9yrzpqzd3ryx",
      // This is a valid Bech32m address, but has can't be converted from 5 to 8
      // bits base which we don't need to support.
      // "11llllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllludsr8",
      "split1checkupstagehandshakeupstreamerranterredcaperredlc445v",
      "?1v759aa",
  });

  for (auto address : valid_bech32_cases) {
    SCOPED_TRACE(address);
    EXPECT_TRUE(Decode(address));
  }

  auto invalid_bech32_cases = std::to_array<std::string_view>({
      "\x{20}1xj0phk",
      "\x{7F}1g6xzxy",
      "\x{80}1vctc34",
      "an84characterslonghumanreadablepartthatcontainsthetheexcludedcharactersb"
      "ioandnumber11d6pts4",
      "qyrz8wqd2c9m",
      "1qyrz8wqd2c9m",
      "y1b0jsk6g",
      "lt1igcx5c0",
      "in1muywd",
      "mm1crxm3i",
      "au1s5cgom",
      "M1VUXWEZ",
      "16plkw9",
      "1p2gdwpf",
  });

  for (auto address : invalid_bech32_cases) {
    SCOPED_TRACE(address);
    EXPECT_FALSE(Decode(address));
  }

  const auto segwit_conversion_cases = std::to_array<
      std::pair<std::string_view, std::string_view>>({
      {"bc1qw508d6qejxtdg4y5r3zarvary0c5xw7kv8f3t4",
       "0014"
       "751e76e8199196d454941c45d1b3a323f1433bd6"},
      {
          "tb1qrp33g0q5c5txsp9arysrx4k6zdkfs4nce4xj0gdcccefvpysxf3q0sl5k7",
          "0020"
          "1863143c14c5166804bd19203356da136c985678cd4d27a1b8c6329604903262",
      },
      {
          "bc1pw508d6qejxtdg4y5r3zarvary0c5xw7kw508d6qejxtdg4y5r3zarvary0c5xw7k"
          "t5nd6y",
          "5128"
          "751e76e8199196d454941c45d1b3a323f1433bd6751e76e8199196d454941c45d1b3"
          "a323f1433bd6",
      },
      {
          "bc1sw50qgdz25j",
          "6002"
          "751e",
      },
      {
          "bc1zw508d6qejxtdg4y5r3zarvaryvaxxpcs",
          "5210"
          "751e76e8199196d454941c45d1b3a323",
      },
      {
          "tb1qqqqqp399et2xygdj5xreqhjjvcmzhxw4aywxecjdzew6hylgvsesrxh6hy",
          "0020"
          "000000c4a5cad46221b2a187905e5266362b99d5e91c6ce24d165dab93e86433",
      },
      {
          "tb1pqqqqp399et2xygdj5xreqhjjvcmzhxw4aywxecjdzew6hylgvsesf3hn0c",
          "5120"
          "000000c4a5cad46221b2a187905e5266362b99d5e91c6ce24d165dab93e86433",
      },
      {
          "bc1p0xlxvlhemja6c4dqv22uapctqupfhlxm9h8z3k2e72q4k9hcz7vqzk5jj0",
          "5120"
          "79be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798",
      },
  });

  for (auto& test_case : segwit_conversion_cases) {
    auto address = test_case.first;
    SCOPED_TRACE(address);

    // Cut off 2 bytes of opcodes from `script_pub_key`.
    auto bytes_hex = test_case.second.substr(4);

    // Test that we can encode/decode it as a bitcoin bech32 address.
    auto decoded = DecodeForBitcoin(address);
    ASSERT_TRUE(decoded);
    EXPECT_EQ(base::HexEncodeLower(decoded->data), bytes_hex);
    EXPECT_EQ(EncodeForBitcoin(decoded->data, decoded->hrp, decoded->witness),
              address);
  }
}

TEST(Bech32UnitTest, EncodeDecode) {
  {
    std::array<uint8_t, 4> payload = {0, 1, 2, 3};
    auto encoded = Encode(payload, "hrp", Encoding::kBech32);
    EXPECT_EQ(encoded, "hrp1qqqsyqclh5s2z");

    auto decoded = Decode(encoded);
    ASSERT_TRUE(decoded);
    EXPECT_EQ(decoded->hrp, "hrp");
    EXPECT_EQ(decoded->encoding, Encoding::kBech32);
    EXPECT_EQ(base::span(decoded->data), base::span(payload));
    EXPECT_EQ(decoded->witness, 0);
  }

  {
    std::array<uint8_t, 4> payload = {0, 1, 2, 3};
    auto encoded = Encode(payload, "hrp", Encoding::kBech32m);
    EXPECT_EQ(encoded, "hrp1qqqsyqc2tyu0q");

    auto decoded = Decode(encoded);
    ASSERT_TRUE(decoded);
    EXPECT_EQ(decoded->hrp, "hrp");
    EXPECT_EQ(decoded->encoding, Encoding::kBech32m);
    EXPECT_EQ(base::span(decoded->data), base::span(payload));
    EXPECT_EQ(decoded->witness, 0);
  }
}

TEST(Bech32UnitTest, EncodeDecodeForBitcoin) {
  {
    std::array<uint8_t, 4> payload = {0, 1, 2, 3};
    auto encoded = EncodeForBitcoin(payload, "hrp", 0);
    EXPECT_EQ(encoded, "hrp1qqqqsyqc4clnsl");

    auto decoded = DecodeForBitcoin(encoded);
    ASSERT_TRUE(decoded);
    EXPECT_EQ(decoded->hrp, "hrp");
    EXPECT_EQ(decoded->encoding, Encoding::kBech32);
    EXPECT_EQ(base::span(decoded->data), base::span(payload));
    EXPECT_EQ(decoded->witness, 0);
  }

  {
    std::array<uint8_t, 4> payload = {0, 1, 2, 3};
    auto encoded = EncodeForBitcoin(payload, "hrp", 7);
    EXPECT_EQ(encoded, "hrp18qqqsyqcpt0nyz");

    auto decoded = DecodeForBitcoin(encoded);
    ASSERT_TRUE(decoded);
    EXPECT_EQ(decoded->hrp, "hrp");
    EXPECT_EQ(decoded->encoding, Encoding::kBech32m);
    EXPECT_EQ(base::span(decoded->data), base::span(payload));
    EXPECT_EQ(decoded->witness, 7);
  }
}

}  // namespace brave_wallet::bech32
