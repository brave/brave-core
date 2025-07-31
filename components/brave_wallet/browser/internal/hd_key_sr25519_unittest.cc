// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_wallet/browser/internal/hd_key_sr25519.h"

#include <type_traits>
#include <utility>
#include <vector>

#include "base/strings/string_number_conversions.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

inline constexpr uint8_t kSchnorrkelSeed[] = {
    157, 97,  177, 157, 239, 253, 90, 96,  186, 132, 74,
    244, 146, 236, 44,  196, 68,  73, 197, 105, 123, 50,
    105, 25,  112, 59,  172, 3,   28, 174, 127, 96,
};

TEST(HDKeySr25519, GenerateFromSeed) {
  auto keypair = HDKeySr25519::GenerateFromSeed(kSchnorrkelSeed);

  std::array<uint8_t, kSr25519SeedSize> seed_bytes = {};
  auto keypair2 = HDKeySr25519::GenerateFromSeed(seed_bytes);
}

TEST(HDKeySr25519, GetPublicKey) {
  auto keypair = HDKeySr25519::GenerateFromSeed(kSchnorrkelSeed);

  auto public_key = keypair.GetPublicKey();
  EXPECT_EQ(public_key.size(), std::size_t{32});

  auto keypair2 = HDKeySr25519::GenerateFromSeed(kSchnorrkelSeed);
  EXPECT_EQ(keypair.GetPublicKey(), keypair2.GetPublicKey());
}

TEST(HDKeySr25519, NoThrowMoveSemantics) {
  EXPECT_TRUE(std::is_nothrow_move_constructible_v<HDKeySr25519>);
  EXPECT_TRUE(std::is_nothrow_move_assignable_v<HDKeySr25519>);
}

TEST(HDKeySr25519, MoveConstruction) {
  auto keypair = HDKeySr25519::GenerateFromSeed(kSchnorrkelSeed);

  HDKeySr25519 keypair2(std::move(keypair));
  auto public_key = keypair2.GetPublicKey();
  EXPECT_EQ(public_key.size(), std::size_t{32});
}

TEST(HDKeySr25519, SelfMoveAssign) {
  // prove that self-move-assign doesn't segfault
  auto keypair = HDKeySr25519::GenerateFromSeed(kSchnorrkelSeed);

  auto& ref = keypair;
  keypair = std::move(ref);
  auto public_key = keypair.GetPublicKey();
  EXPECT_EQ(public_key.size(), std::size_t{32});
}

TEST(HDKeySr25519, MoveAssignment) {
  auto keypair1 = HDKeySr25519::GenerateFromSeed(kSchnorrkelSeed);
  auto pubkey1 = keypair1.GetPublicKey();

  std::array<uint8_t, kSr25519SeedSize> seed = {};
  auto keypair2 = HDKeySr25519::GenerateFromSeed(seed);

  EXPECT_NE(pubkey1, keypair2.GetPublicKey());

  keypair2 = std::move(keypair1);
  EXPECT_EQ(pubkey1, keypair2.GetPublicKey());
}

TEST(HDKeySr25519, SignAndVerify) {
  auto keypair = HDKeySr25519::GenerateFromSeed(kSchnorrkelSeed);

  auto public_key = keypair.GetPublicKey();
  EXPECT_EQ(public_key.size(), std::size_t{32});

  unsigned char const message[] = {1, 2, 3, 4, 5, 6};
  auto sig = keypair.SignMessage(message);

  auto is_verified = keypair.VerifyMessage(sig, message);
  EXPECT_TRUE(is_verified);

  std::array<uint8_t, 64> bad_sig = {};
  is_verified = keypair.VerifyMessage(bad_sig, message);
  EXPECT_FALSE(is_verified);

  std::array<uint8_t, 64> bad_message = {};
  is_verified = keypair.VerifyMessage(sig, bad_message);
  EXPECT_FALSE(is_verified);
}

TEST(HDKeySr25519, HardDerive) {
  auto keypair = HDKeySr25519::GenerateFromSeed(kSchnorrkelSeed);

  // Manually create a SCALE-encoded chaincode values for deriving child
  // keypairs from a parent.
  //
  // When it comes to deriving chaincodes for deriving child keypairs from a
  // path like `<mnemonic>//Alice`, the polkdadot-sdk does this:
  //
  // https://github.com/paritytech/polkadot-sdk/blob/7304295748b1d85eb9fc2b598eba43d9f7971f22/substrate/primitives/core/src/crypto.rs#L820
  // https://github.com/paritytech/polkadot-sdk/blob/7304295748b1d85eb9fc2b598eba43d9f7971f22/substrate/primitives/core/src/crypto.rs#L185
  // https://github.com/paritytech/polkadot-sdk/blob/7304295748b1d85eb9fc2b598eba43d9f7971f22/substrate/primitives/core/src/crypto.rs#L138-L151
  //
  // The important call is: index.using_encoded(|data| { ... })
  //
  // `index.using_encoded()` invokes the provided lambda with a SCALE-encoded
  // version of the `index`. In our case, we simply prepend a length prefix
  // manually that matches what the polkadot-sdk calculates. Someday we'll need
  // to have our own SCALE routines.
  //
  // The encoding routines live here as a separate crate:
  // https://github.com/paritytech/parity-scale-codec/blob/cbb20a746ef1db377f4c1df54ab89da6ebc316f4/src/codec.rs#L1105-L1115
  //
  // The routines work without explicit SCALE coding but it means our results
  // will diverge if we update these to match test vectors from the polkadot-sdk
  // from paritytech.
  //
  // See also:
  // https://wiki.polkadot.com/learn/learn-account-advanced/#soft-and-hard-derivation
  //
  unsigned char path1[] = {20, 'A', 'l', 'i', 'c', 'e'};
  unsigned char path2[] = {20, 'e', 'c', 'i', 'l', 'A'};

  auto derived1 = keypair.DeriveHard(path1);
  auto derived2 = keypair.DeriveHard(path2);
  auto derived3 = keypair.DeriveHard(path1);

  EXPECT_EQ(derived1.GetPublicKey(), derived3.GetPublicKey());
  EXPECT_NE(derived1.GetPublicKey(), derived2.GetPublicKey());

  auto keypair2 = HDKeySr25519::GenerateFromSeed(
      base::span<const uint8_t, kSr25519SeedSize>{
          250, 199, 149, 157, 191, 231, 47,  5,   46,  90,  12,
          60,  141, 101, 48,  242, 2,   176, 47,  216, 249, 245,
          202, 53,  128, 236, 141, 235, 119, 151, 71,  158});

  auto derived4 = keypair2.DeriveHard(path1);
  auto derived5 = keypair2.DeriveHard(path2);

  EXPECT_NE(derived4.GetPublicKey(), derived1.GetPublicKey());
  EXPECT_NE(derived5.GetPublicKey(), derived2.GetPublicKey());

  auto grandchild = derived1.DeriveHard(path2);
  EXPECT_NE(grandchild.GetPublicKey(), derived1.GetPublicKey());
  EXPECT_NE(grandchild.GetPublicKey(), derived2.GetPublicKey());
}

TEST(HDKeySr25519, HardDeriveSignAndVerify) {
  auto keypair = HDKeySr25519::GenerateFromSeed(kSchnorrkelSeed);

  // manually create a SCALE-encoded chaincode value
  unsigned char path1[] = {20, 'A', 'l', 'i', 'c', 'e'};
  unsigned char path2[] = {20, 'e', 'c', 'i', 'l', 'A'};

  auto derived1 = keypair.DeriveHard(path1);
  auto derived2 = keypair.DeriveHard(path2);
  auto derived3 = keypair.DeriveHard(path1);

  unsigned char const message[] = {1, 2, 3, 4, 5, 6};
  auto signature = derived1.SignMessage(message);

  EXPECT_FALSE(derived2.VerifyMessage(signature, message));
  EXPECT_TRUE(derived3.VerifyMessage(signature, message));
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
  auto keypair = HDKeySr25519::GenerateFromSeed(
      base::span<const uint8_t, kSr25519SeedSize>{
          250, 199, 149, 157, 191, 231, 47,  5,   46,  90,  12,
          60,  141, 101, 48,  242, 2,   176, 47,  216, 249, 245,
          202, 53,  128, 236, 141, 235, 119, 151, 71,  158});

  // manually create a SCALE-encoded chaincode value
  unsigned char path[] = {20, 'A', 'l', 'i', 'c', 'e'};

  std::vector<uint8_t> expected;
  base::HexStringToBytes(
      "d43593c715fdd31c61141abd04a99fd6822c8558854ccde39a5684e7a56da27d",
      &expected);

  auto derived = keypair.DeriveHard(path);
  auto pkey = derived.GetPublicKey();
  EXPECT_EQ(base::span{pkey}, base::span{expected});

  // Now test the blake2 hashing portion given a sufficiently long derive
  // junction.
  // Because this test isn't a formal vector, we generate it manually via:
  //
  // let pair1 = polkadot_sdk::sp_core::sr25519::Pair::from_string(
  //   &format!("{}//AnIncrediblyLongDerivationPathNameToTriggerBlake2",
  //   polkadot_sdk::sp_core::crypto::DEV_PHRASE), None,
  //  )
  // .unwrap();
  //
  // let expected =
  //   hex_to_bytes("225ba704a8fb5acfadb790e41cda8c8f75698e6f1fd5a99a5bd2183b9b899857").unwrap();
  // assert_eq!(pair1.public().as_slice(), &expected);

  expected.clear();
  base::HexStringToBytes(
      "225ba704a8fb5acfadb790e41cda8c8f75698e6f1fd5a99a5bd2183b9b899857",
      &expected);

  // Rotely copy the SCALE-encoded version of the string:
  // "AnIncrediblyLongDerivationPathNameToTriggerBlake2"
  unsigned char long_path[] = {196, 65,  110, 73,  110, 99,  114, 101, 100, 105,
                               98,  108, 121, 76,  111, 110, 103, 68,  101, 114,
                               105, 118, 97,  116, 105, 111, 110, 80,  97,  116,
                               104, 78,  97,  109, 101, 84,  111, 84,  114, 105,
                               103, 103, 101, 114, 66,  108, 97,  107, 101, 50};
  derived = keypair.DeriveHard(long_path);
  pkey = derived.GetPublicKey();
  EXPECT_EQ(base::span{pkey}, base::span{expected});
}

TEST(HDKeySr25519, PolkadotSDKTestVector2) {
  // https://github.com/paritytech/polkadot-sdk/blob/40e1a2a7c99c67fe5201145e473c87e1aea4bf05/substrate/primitives/core/src/sr25519.rs#L714

  std::vector<uint8_t> seed;
  base::HexStringToBytes(
      "9d61b19deffd5a60ba844af492ec2cc44449c5697b326919703bac031cae7f60",
      &seed);

  auto keypair = HDKeySr25519::GenerateFromSeed(
      base::span<const uint8_t, kSr25519SeedSize>(seed));

  auto pkey = keypair.GetPublicKey();

  std::vector<uint8_t> expected_pkey;
  base::HexStringToBytes(
      "44a996beb1eef7bdcab976ab6d2ca26104834164ecf28fb375600576fcc6eb0f",
      &expected_pkey);

  EXPECT_EQ(base::span{pkey}, base::span{expected_pkey});
}

}  // namespace brave_wallet
