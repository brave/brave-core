/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/encoding_utils.h"

#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

TEST(EncodingUtilsUnitTest, Base58Encode) {
  EXPECT_EQ("2g", Base58Encode(PrefixedHexStringToBytes("0x61").value()));
  EXPECT_EQ("a3gV", Base58Encode(PrefixedHexStringToBytes("0x626262").value()));
  EXPECT_EQ("aPEr", Base58Encode(PrefixedHexStringToBytes("0x636363").value()));
  EXPECT_EQ("2cFupjhnEsSn59qHXstmK2ffpLv2",
            Base58Encode(PrefixedHexStringToBytes(
                             "0x73696d706c792061206c6f6e6720737472696e67")
                             .value()));
  EXPECT_EQ("ABnLTmg",
            Base58Encode(PrefixedHexStringToBytes("0x516b6fcd0f").value()));
  EXPECT_EQ("AjUL4MR",
            Base58Encode(PrefixedHexStringToBytes("0x5643312234").value()));
}

TEST(EncodingUtilsUnitTest, Base58Decode) {
  {
    std::vector<uint8_t> bytes;
    Base58Decode("2g", &bytes, 1);
    EXPECT_EQ("0x61", ToHex(bytes));
  }

  {
    std::vector<uint8_t> bytes;
    Base58Decode("a3gV", &bytes, 3);
    EXPECT_EQ("0x626262", ToHex(bytes));
  }

  {
    std::vector<uint8_t> bytes;
    Base58Decode("aPEr", &bytes, 3);
    EXPECT_EQ("0x636363", ToHex(bytes));
  }

  {
    std::vector<uint8_t> bytes;
    Base58Decode("2cFupjhnEsSn59qHXstmK2ffpLv2", &bytes, 20);
    EXPECT_EQ("0x73696d706c792061206c6f6e6720737472696e67", ToHex(bytes));
  }
}

TEST(EncodingUtilsUnitTest, Base58EncodeWithCheck) {
  EXPECT_EQ(
      "1AGNa15ZQXAZUgFiqJ2i7Z2DPU2J6hW62i",
      Base58EncodeWithCheck(PrefixedHexStringToBytes(
                                "0x0065a16059864a2fdbc7c99a4723a8395bc6f188eb")
                                .value()));
  EXPECT_EQ(
      "3CMNFxN1oHBc4R1EpboAL5yzHGgE611Xou",
      Base58EncodeWithCheck(PrefixedHexStringToBytes(
                                "0x0574f209f6ea907e2ea48f74fae05782ae8a665257")
                                .value()));
  EXPECT_EQ(
      "mo9ncXisMeAoXwqcV5EWuyncbmCcQN4rVs",
      Base58EncodeWithCheck(PrefixedHexStringToBytes(
                                "0x6f53c0307d6851aa0ce7825ba883c6bd9ad242b486")
                                .value()));
  EXPECT_EQ(
      "2N2JD6wb56AfK4tfmM6PwdVmoYk2dCKf4Br",
      Base58EncodeWithCheck(PrefixedHexStringToBytes(
                                "0xc46349a418fc4578d10a372b54b45c280cc8c4382f")
                                .value()));
  EXPECT_EQ("5Kd3NBUAdUnhyzenEwVLy9pBKxSwXvE9FMPyR4UKZvpe6E3AgLr",
            Base58EncodeWithCheck(
                PrefixedHexStringToBytes("0x80eddbdc1168f1daeadbd3e44c1e3f8f5a2"
                                         "84c2029f78ad26af98583a499de5b19")
                    .value()));
}

// Test vectors calculated using https://ss58.org
TEST(EncodingUtilsUnitTest, Ss58Encode) {
  // Single-byte prefix
  {
    Ss58Address addr;
    EXPECT_TRUE(base::HexStringToSpan(
        "d43593c715fdd31c61141abd04a99fd6822c8558854ccde39a5684e7a56da27d",
        base::span(addr.public_key)));
    addr.prefix = 0;
    EXPECT_EQ("15oF4uVJwmo4TdGW7VfQxNLavjCXviqxT9S1MgbjMNHr6Sp5",
              addr.Encode().value());

    auto decoded =
        Ss58Address::Decode("15oF4uVJwmo4TdGW7VfQxNLavjCXviqxT9S1MgbjMNHr6Sp5");
    EXPECT_EQ(decoded->public_key, addr.public_key);
    EXPECT_EQ(decoded->prefix, 0u);
  }

  {
    Ss58Address addr;
    EXPECT_TRUE(base::HexStringToSpan(
        "d43593c715fdd31c61141abd04a99fd6822c8558854ccde39a5684e7a56da27d",
        base::span(addr.public_key)));
    addr.prefix = 42;
    EXPECT_EQ("5GrwvaEF5zXb26Fz9rcQpDWS57CtERHpNehXCPcNoHGKutQY",
              addr.Encode().value());

    auto decoded =
        Ss58Address::Decode("5GrwvaEF5zXb26Fz9rcQpDWS57CtERHpNehXCPcNoHGKutQY");
    EXPECT_EQ(decoded->public_key, addr.public_key);
    EXPECT_EQ(decoded->prefix, 42u);
  }

  {
    Ss58Address addr;
    EXPECT_TRUE(base::HexStringToSpan(
        "d43593c715fdd31c61141abd04a99fd6822c8558854ccde39a5684e7a56da27d",
        base::span(addr.public_key)));
    addr.prefix = 63;
    EXPECT_EQ("7NPoMQbiA6trJKkjB35uk96MeJD4PGWkLQLH7k7hXEkZpiba",
              addr.Encode().value());

    auto decoded =
        Ss58Address::Decode("7NPoMQbiA6trJKkjB35uk96MeJD4PGWkLQLH7k7hXEkZpiba");
    EXPECT_EQ(decoded->public_key, addr.public_key);
    EXPECT_EQ(decoded->prefix, 63u);
  }

  // Two-bytes prefix
  {
    Ss58Address addr;
    EXPECT_TRUE(base::HexStringToSpan(
        "d43593c715fdd31c61141abd04a99fd6822c8558854ccde39a5684e7a56da27d",
        base::span(addr.public_key)));
    addr.prefix = 64u;
    EXPECT_EQ("cEaNSpz4PxFcZ7nT1VEKrKewH67rfx6MfcM6yKojyyPz7qaqp",
              addr.Encode().value());

    auto decoded = Ss58Address::Decode(
        "cEaNSpz4PxFcZ7nT1VEKrKewH67rfx6MfcM6yKojyyPz7qaqp");
    EXPECT_EQ(decoded->public_key, addr.public_key);
    EXPECT_EQ(decoded->prefix, 64u);
  }

  {
    Ss58Address addr;
    EXPECT_TRUE(base::HexStringToSpan(
        "d43593c715fdd31c61141abd04a99fd6822c8558854ccde39a5684e7a56da27d",
        base::span(addr.public_key)));
    addr.prefix = 200;
    EXPECT_EQ("sD4TrgAhf8c8tYheiyKQb19jvTWY5fGx3TD1nK1Ue61WHRnT4",
              addr.Encode().value());

    auto decoded =
        Ss58Address::Decode("5GrwvaEF5zXb26Fz9rcQpDWS57CtERHpNehXCPcNoHGKutQY");
    EXPECT_EQ(decoded->public_key, addr.public_key);
    EXPECT_EQ(decoded->prefix, 42u);
  }

  {
    Ss58Address addr;
    EXPECT_TRUE(base::HexStringToSpan(
        "d43593c715fdd31c61141abd04a99fd6822c8558854ccde39a5684e7a56da27d",
        base::span(addr.public_key)));
    addr.prefix = 16383;
    EXPECT_EQ("yNa8JpqfFB3q8A29rCwSgxvdU94ufJw2yKKxDgznS5m1PoFvn",
              addr.Encode().value());

    auto decoded = Ss58Address::Decode(
        "yNa8JpqfFB3q8A29rCwSgxvdU94ufJw2yKKxDgznS5m1PoFvn");
    EXPECT_EQ(decoded->public_key, addr.public_key);
    EXPECT_EQ(decoded->prefix, 16383u);
  }

  // Wrong prefix encode
  {
    Ss58Address addr;
    EXPECT_TRUE(base::HexStringToSpan(
        "d43593c715fdd31c61141abd04a99fd6822c8558854ccde39a5684e7a56da27d",
        base::span(addr.public_key)));
    addr.prefix = 16384u;
    EXPECT_FALSE(addr.Encode());
  }

  // Wrong prefix decode
  {
    EXPECT_FALSE(Ss58Address::Decode(
        "HHrwvaEF5zXb26Fz9rcQpDWS57CtERHpNehXCPcNoHGKutQY"));
  }

  // Wrong addr size
  {
    EXPECT_FALSE(Ss58Address::Decode(
        "0Na8JpqfFB3q8A29rCwSgxvdU94ufJw2yKKxDgznS5m1PoFvn"));
  }

  // Wrong checksum
  {
    EXPECT_FALSE(Ss58Address::Decode(
        "5GrwvaEF5zXb26Fz9rcQpDWS57CtERHpNehXCPcNoHGKutQa"));
  }
}

}  // namespace brave_wallet
