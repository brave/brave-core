/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/hash_utils.h"

#include <vector>

#include "base/containers/span.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

TEST(HashUtilsUnitTest, KeccakHash) {
  ASSERT_EQ(
      ToHex(KeccakHash({})),
      "0xc5d2460186f7233c927e7db2dcc703c0e500b653ca82273b7bfad8045d85a470");
  ASSERT_EQ(
      ToHex(KeccakHash(base::byte_span_from_cstring("hello world"))),
      "0x47173285a8d7341e5e972fc677286384f802f8ef42a5ec5f03bbfa254cb01fad");
}

TEST(HashUtilsUnitTest, GetFunctionHash) {
  ASSERT_EQ(GetFunctionHash("transfer(address,uint256)"), "0xa9059cbb");
  ASSERT_EQ(GetFunctionHash("approve(address,uint256)"), "0x095ea7b3");
  ASSERT_EQ(GetFunctionHash("balanceOf(address)"), "0x70a08231");
}

TEST(HashUtilsUnitTest, Namehash) {
  EXPECT_EQ(
      ToHex(Namehash("")),
      "0x0000000000000000000000000000000000000000000000000000000000000000");
  EXPECT_EQ(
      ToHex(Namehash("eth")),
      "0x93cdeb708b7545dc668eb9280176169d1c33cfd8ed6f04690a0bcc88a93fc4ae");
  EXPECT_EQ(
      ToHex(Namehash("foo.eth")),
      "0xde9b09fd7c5f901e23a3f19fecc54828e9c848539801e86591bd9801b019f84f");
  EXPECT_EQ(
      ToHex(Namehash(".")),
      "0x0000000000000000000000000000000000000000000000000000000000000000");
  EXPECT_EQ(
      ToHex(Namehash("crypto")),
      "0x0f4a10a4f46c288cea365fcf45cccf0e9d901b945b9829ccdb54c10dc3cb7a6f");
  EXPECT_EQ(
      ToHex(Namehash("example.crypto")),
      "0xd584c5509c6788ad9d9491be8ba8b4422d05caf62674a98fbf8a9988eeadfb7e");
  EXPECT_EQ(
      ToHex(Namehash("www.example.crypto")),
      "0x3ae54ac25ccd63401d817b6d79a4a56ae7f79a332fe77a98fa0c9d10adf9b2a1");
  EXPECT_EQ(
      ToHex(Namehash("a.b.c.crypto")),
      "0x353ea3e0449067382e0ea7934767470170dcfa9c49b1be0fe708adc4b1f9cf13");
  EXPECT_EQ(
      ToHex(Namehash("brave.crypto")),
      "0x77252571a99feee8f5e6b2f0c8b705407d395adc00b3c8ebcc7c19b2ea850013");
}

TEST(HashUtilsUnitTest, DoubleSHA256Hash) {
  // https://seclists.org/nmap-dev/2012/q4/att-514/SHAd256_Test_Vectors.txt

  // NIST.1
  EXPECT_EQ(
      HexEncodeLower(DoubleSHA256Hash(std::vector<uint8_t>{0x61, 0x62, 0x63})),
      "4f8b42c22dd3729b519ba6f68d2da7cc5b2d606d05daed5ad5128cc03e6c6358");

  // EMPTY
  EXPECT_EQ(HexEncodeLower(DoubleSHA256Hash(std::vector<uint8_t>{})),
            "5df6e0e2761359d30a8275058e299fcc0381534545f55cf43e41983f5d4c9456");
}

TEST(HashUtilsUnitTest, Hash160) {
  EXPECT_EQ(HexEncodeLower(Hash160(std::vector<uint8_t>{0x61, 0x62, 0x63})),
            "bb1be98c142444d7a56aa3981c3942a978e4dc33");

  EXPECT_EQ(HexEncodeLower(Hash160(std::vector<uint8_t>{})),
            "b472a266d0bd89c13706a4132ccfb16f7c3b9fcb");
}

TEST(HashUtilsUnitTest, HmacSha512) {
  // Empty vectors test.
  EXPECT_EQ(HexEncodeLower(HmacSha512({}, {})),
            "b936cee86c9f87aa5d3c6f2e84cb5a4239a5fe50480a6ec66b70ab5b1f4ac673"
            "0c6c515421b327ec1d69402e53dfb49ad7381eb067b338fd7b0cb22247225d47");

  // Large vectors test.
  EXPECT_EQ(HexEncodeLower(HmacSha512(std::vector<uint8_t>(1000, 0xee),
                                      std::vector<uint8_t>(2000, 0x45))),
            "5d6a801cf32c7d5edb17f5287653c86323599de6e8ab76819b3530494e144ec6"
            "3a40f6e541d6cc8a7db3d0560349d74ca52c1e370c9a70a96096e28761d017fc");

  // https://datatracker.ietf.org/doc/html/rfc4231#section-4.2
  EXPECT_EQ(
      HexEncodeLower(HmacSha512(std::vector<uint8_t>(20, 0x0b),
                                base::byte_span_from_cstring("Hi There"))),
      "87aa7cdea5ef619d4ff0b4241a1d6cb02379f4e2ce4ec2787ad0b30545e17cde"
      "daa833b7d6b8a702038b274eaea3f4e4be9d914eeb61f1702e696c203a126854");

  // https://datatracker.ietf.org/doc/html/rfc4231#section-4.4
  EXPECT_EQ(HexEncodeLower(HmacSha512(std::vector<uint8_t>(20, 0xaa),
                                      std::vector<uint8_t>(50, 0xdd))),
            "fa73b0089d56a284efb0f0756c890be9b1b5dbdd8ee81a3655f83e33b2279d39"
            "bf3e848279a722c806b485a47e67c807b946a337bee8942674278859e13292fb");
}

}  // namespace brave_wallet
