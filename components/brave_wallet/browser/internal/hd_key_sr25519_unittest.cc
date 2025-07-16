// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_wallet/browser/internal/hd_key_sr25519.h"

#include "base/strings/string_number_conversions.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

inline constexpr uint8_t kSchnorrkelSeed[] = {
    157, 97,  177, 157, 239, 253, 90, 96,  186, 132, 74,
    244, 146, 236, 44,  196, 68,  73, 197, 105, 123, 50,
    105, 25,  112, 59,  172, 3,   28, 174, 127, 96,
};

TEST(HDKeySr25519, GenerateFromSeed) {
  auto kpresult = HDKeySr25519::GenerateFromSeed({});
  EXPECT_FALSE(kpresult);

  kpresult = HDKeySr25519::GenerateFromSeed({1, 2, 3, 4});
  EXPECT_FALSE(kpresult);

  kpresult = HDKeySr25519::GenerateFromSeed(kSchnorrkelSeed);
  EXPECT_TRUE(kpresult);
}

TEST(HDKeySr25519, GetPublicKey) {
  auto kpresult = HDKeySr25519::GenerateFromSeed(kSchnorrkelSeed);
  EXPECT_TRUE(kpresult);

  auto public_key = kpresult->GetPublicKey();
  EXPECT_EQ(public_key.size(), std::size_t{32});
}

TEST(HDKeySr25519, SignAndVerify) {
  auto kpresult = HDKeySr25519::GenerateFromSeed(kSchnorrkelSeed);
  EXPECT_TRUE(kpresult);

  auto public_key = kpresult->GetPublicKey();
  EXPECT_EQ(public_key.size(), std::size_t{32});

  unsigned char const message[] = {1, 2, 3, 4, 5, 6};
  auto sig = kpresult->SignMessage(message);

  auto is_verified = kpresult->VerifyMessage(sig, message);
  EXPECT_TRUE(is_verified);

  std::array<uint8_t, 64> bad_sig = {};
  is_verified = kpresult->VerifyMessage(bad_sig, message);
  EXPECT_FALSE(is_verified);

  std::array<uint8_t, 64> bad_message = {};
  is_verified = kpresult->VerifyMessage(sig, bad_message);
  EXPECT_FALSE(is_verified);
}

TEST(HDKeySr25519, PolkadotSDKTestVector1) {
  // Trying to pass the same test here:
  // https://github.com/paritytech/polkadot-sdk/blob/40e1a2a7c99c67fe5201145e473c87e1aea4bf05/substrate/primitives/core/src/sr25519.rs#L614

  // The polkadot-sdk seems to actually hash the entropy bytes themselves when
  // generating a seed, which we don't do. To make our test vector match theirs,
  // we just copy-paste the seed as-is when they generate the MiniSecretKey from
  // it.
  // https://github.com/paritytech/polkadot-sdk/blob/40e1a2a7c99c67fe5201145e473c87e1aea4bf05/substrate/utils/substrate-bip39/src/lib.rs#L52-L70
  // https://github.com/paritytech/polkadot-sdk/blob/40e1a2a7c99c67fe5201145e473c87e1aea4bf05/substrate/primitives/core/src/crypto.rs#L875
  auto kpresult = HDKeySr25519::GenerateFromSeed(
      {250, 199, 149, 157, 191, 231, 47,  5,   46,  90,  12,
       60,  141, 101, 48,  242, 2,   176, 47,  216, 249, 245,
       202, 53,  128, 236, 141, 235, 119, 151, 71,  158});
  EXPECT_TRUE(kpresult);

  // manually create a SCALE-encoded chaincode value
  unsigned char path[] = {20, 'A', 'l', 'i', 'c', 'e'};

  std::vector<uint8_t> expected;
  base::HexStringToBytes(
      "d43593c715fdd31c61141abd04a99fd6822c8558854ccde39a5684e7a56da27d",
      &expected);

  auto derived = kpresult->DeriveHard(path);
  auto pkey = derived->GetPublicKey();
  EXPECT_EQ(base::span{pkey}, base::span{expected});
}

TEST(HDKeySr25519, PolkadotSDKTestVector2) {
  // https://github.com/paritytech/polkadot-sdk/blob/40e1a2a7c99c67fe5201145e473c87e1aea4bf05/substrate/primitives/core/src/sr25519.rs#L714

  std::vector<uint8_t> seed;
  base::HexStringToBytes(
      "9d61b19deffd5a60ba844af492ec2cc44449c5697b326919703bac031cae7f60",
      &seed);

  auto kpresult = HDKeySr25519::GenerateFromSeed(seed);
  ASSERT_TRUE(kpresult);

  auto pkey = kpresult->GetPublicKey();

  std::vector<uint8_t> expected_pkey;
  base::HexStringToBytes(
      "44a996beb1eef7bdcab976ab6d2ca26104834164ecf28fb375600576fcc6eb0f",
      &expected_pkey);

  EXPECT_EQ(base::span{pkey}, base::span{expected_pkey});
}

}  // namespace brave_wallet
