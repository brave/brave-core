// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_wallet/browser/internal/hd_key_sr25519.h"

#include <type_traits>
#include <utility>
#include <vector>

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace {

// Taken from:
// https://docs.rs/schnorrkel/0.11.4/schnorrkel/keys/struct.MiniSecretKey.html#method.from_bytes
constexpr uint8_t kSchnorrkelSeed[] = {
    157, 97,  177, 157, 239, 253, 90, 96,  186, 132, 74,
    244, 146, 236, 44,  196, 68,  73, 197, 105, 123, 50,
    105, 25,  112, 59,  172, 3,   28, 174, 127, 96,
};

// Manually derived from the polkadot-sdk using
// `polkadot_sdk::sp_core::sr25519::Pair`.
constexpr const char kSchnorrkelPubKey[] =
    "44A996BEB1EEF7BDCAB976AB6D2CA26104834164ECF28FB375600576FCC6EB0F";

}  // namespace

TEST(HDKeySr25519, GenerateFromSeed) {
  auto keypair = HDKeySr25519::GenerateFromSeed(kSchnorrkelSeed);

  std::array<uint8_t, kSr25519SeedSize> seed_bytes = {};
  auto keypair2 = HDKeySr25519::GenerateFromSeed(seed_bytes);
}

TEST(HDKeySr25519, GetPublicKey) {
  auto keypair = HDKeySr25519::GenerateFromSeed(kSchnorrkelSeed);
  auto pubkey = base::HexEncode(keypair.GetPublicKey());
  EXPECT_EQ(pubkey, kSchnorrkelPubKey);

  // Prove idempotence.
  auto keypair2 = HDKeySr25519::GenerateFromSeed(kSchnorrkelSeed);
  auto pubkey2 = base::HexEncode(keypair2.GetPublicKey());
  EXPECT_EQ(pubkey2, kSchnorrkelPubKey);
}

TEST(HDKeySr25519, NoThrowMoveSemantics) {
  EXPECT_TRUE(std::is_nothrow_move_constructible_v<HDKeySr25519>);
  EXPECT_TRUE(std::is_nothrow_move_assignable_v<HDKeySr25519>);
}

TEST(HDKeySr25519, MoveConstruction) {
  auto keypair = HDKeySr25519::GenerateFromSeed(kSchnorrkelSeed);

  HDKeySr25519 keypair2(std::move(keypair));
  auto pubkey = base::HexEncode(keypair2.GetPublicKey());
  EXPECT_EQ(pubkey, kSchnorrkelPubKey);
}

TEST(HDKeySr25519, SelfMoveAssign) {
  // Prove that self-move-assign doesn't segfault.
  auto keypair = HDKeySr25519::GenerateFromSeed(kSchnorrkelSeed);

  auto& ref = keypair;
  keypair = std::move(ref);
  auto pubkey = base::HexEncode(keypair.GetPublicKey());
  EXPECT_EQ(pubkey, kSchnorrkelPubKey);
}

TEST(HDKeySr25519, MoveAssignment) {
  auto keypair1 = HDKeySr25519::GenerateFromSeed(kSchnorrkelSeed);

  std::array<uint8_t, kSr25519SeedSize> seed = {};
  auto keypair2 = HDKeySr25519::GenerateFromSeed(seed);
  constexpr const char* kEmptySeedPubKey =
      "DEF12E42F3E487E9B14095AA8D5CC16A33491F1B50DADCF8811D1480F3FA8627";
  EXPECT_EQ(base::HexEncode(keypair2.GetPublicKey()), kEmptySeedPubKey);

  keypair2 = std::move(keypair1);
  EXPECT_EQ(base::HexEncode(keypair2.GetPublicKey()), kSchnorrkelPubKey);
}

TEST(HDKeySr25519, SignAndVerify) {
  // Schnorr signatures and the schnorrkel crate use a randomized nonce when
  // generating the signature so we can't test against any hard-coded vectors
  // but can only prove that signatures won't match but they'll still verify the
  // same message using the same keypair.

  auto keypair = HDKeySr25519::GenerateFromSeed(kSchnorrkelSeed);
  EXPECT_EQ(base::HexEncode(keypair.GetPublicKey()), kSchnorrkelPubKey);

  unsigned char const message[] = {1, 2, 3, 4, 5, 6};
  auto signature = keypair.SignMessage(message);

  auto is_verified = keypair.VerifyMessage(signature, message);
  EXPECT_TRUE(is_verified);

  auto signature2 = keypair.SignMessage(message);
  is_verified = keypair.VerifyMessage(signature2, message);
  EXPECT_NE(base::HexEncode(signature2), base::HexEncode(signature));
  EXPECT_TRUE(is_verified);

  std::array<uint8_t, 64> bad_sig = {};
  is_verified = keypair.VerifyMessage(bad_sig, message);
  EXPECT_FALSE(is_verified);

  std::array<uint8_t, 64> bad_message = {};
  is_verified = keypair.VerifyMessage(signature, bad_message);
  EXPECT_FALSE(is_verified);
}

TEST(HDKeySr25519, VerifySignature) {
  // Derived from the binary message [1, 2, 3, 4, 6] using our kSchnorrkelSeed.

  constexpr const char* kSchnorrSignature =
      "669DB9831C33855F0A3BFCF0B8F48EDDE504281C5CED4DF7882E0FF89A48F77128DB08B7"
      "B90AE7CDF45602FF0F7C78E49594E282D955C0EDFE9080945703E28F";

  std::vector<uint8_t> signature_bytes;
  base::HexStringToBytes(kSchnorrSignature, &signature_bytes);

  auto keypair = HDKeySr25519::GenerateFromSeed(kSchnorrkelSeed);
  auto is_verified = keypair.VerifyMessage(
      base::span<uint8_t const, kSr25519SignatureSize>(signature_bytes),
      {1, 2, 3, 4, 5, 6});
  EXPECT_TRUE(is_verified);
}

TEST(HDKeySr25519, HardDerive) {
  auto keypair = HDKeySr25519::GenerateFromSeed(kSchnorrkelSeed);
  EXPECT_EQ(base::HexEncode(keypair.GetPublicKey()), kSchnorrkelPubKey);

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

  // Derived using the polkadot-sdk:
  // let derived =
  //   pair.derive(
  //     Some(DeriveJunction::from("Alice").harden()).into_iter(),
  //     None).unwrap().0;
  constexpr const char* kPath1DerivedPubKey =
      "382F0AD81E1820A654E5D461FF4B9FD35B7E714C217B2F1301784A159CE27378";

  EXPECT_EQ(base::HexEncode(derived1.GetPublicKey()), kPath1DerivedPubKey);
  EXPECT_EQ(base::HexEncode(derived3.GetPublicKey()), kPath1DerivedPubKey);

  // Derived similarly above using /ecilA.
  constexpr const char* kPath2DerivedPubKey =
      "F0F4DC4A68BB4977FE41DAC5F6846260F0BAB780F60BDAADB8C37AD95DFBFD10";

  EXPECT_EQ(base::HexEncode(derived2.GetPublicKey()), kPath2DerivedPubKey);

  auto keypair2 = HDKeySr25519::GenerateFromSeed(
      base::span<const uint8_t, kSr25519SeedSize>{
          250, 199, 149, 157, 191, 231, 47,  5,   46,  90,  12,
          60,  141, 101, 48,  242, 2,   176, 47,  216, 249, 245,
          202, 53,  128, 236, 141, 235, 119, 151, 71,  158});

  constexpr const char* kPath1Pair2DerivedPubKey =
      "D43593C715FDD31C61141ABD04A99FD6822C8558854CCDE39A5684E7A56DA27D";

  EXPECT_EQ(base::HexEncode(keypair2.DeriveHard(path1).GetPublicKey()),
            kPath1Pair2DerivedPubKey);

  constexpr const char* kPath2Pair2DerivedPubKey =
      "0823945F7ED05A3FC0F1F4B24F110A8C3CA1260C325274C4A3A4E0AEE38EE12F";

  EXPECT_EQ(base::HexEncode(keypair2.DeriveHard(path2).GetPublicKey()),
            kPath2Pair2DerivedPubKey);

  constexpr char const* kGrandchildPubKey =
      "089A2E5523DEBAE16D260D452AF57E700703F3ADD47DBE62634AFB96C7E4315B";

  auto grandchild = derived1.DeriveHard(path2);
  EXPECT_EQ(base::HexEncode(grandchild.GetPublicKey()), kGrandchildPubKey);
}

TEST(HDKeySr25519, HardDeriveSignAndVerify) {
  auto keypair = HDKeySr25519::GenerateFromSeed(kSchnorrkelSeed);

  // Manually create a SCALE-encoded chaincode value.
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

  // Manually create a SCALE-encoded chaincode value.
  unsigned char path[] = {20, 'A', 'l', 'i', 'c', 'e'};

  constexpr const char* kDerivedPubKey =
      "D43593C715FDD31C61141ABD04A99FD6822C8558854CCDE39A5684E7A56DA27D";

  auto derived = keypair.DeriveHard(path);
  EXPECT_EQ(base::HexEncode(derived.GetPublicKey()), kDerivedPubKey);

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

  constexpr const char* kLongDerivedPubKey =
      "225BA704A8FB5ACFADB790E41CDA8C8F75698E6F1FD5A99A5BD2183B9B899857";

  // Rotely copy the SCALE-encoded version of the string:
  // "AnIncrediblyLongDerivationPathNameToTriggerBlake2"
  unsigned char long_path[] = {196, 65,  110, 73,  110, 99,  114, 101, 100, 105,
                               98,  108, 121, 76,  111, 110, 103, 68,  101, 114,
                               105, 118, 97,  116, 105, 111, 110, 80,  97,  116,
                               104, 78,  97,  109, 101, 84,  111, 84,  114, 105,
                               103, 103, 101, 114, 66,  108, 97,  107, 101, 50};
  derived = keypair.DeriveHard(long_path);
  EXPECT_EQ(base::HexEncode(derived.GetPublicKey()), kLongDerivedPubKey);
}

TEST(HDKeySr25519, PolkadotSDKTestVector2) {
  // https://github.com/paritytech/polkadot-sdk/blob/40e1a2a7c99c67fe5201145e473c87e1aea4bf05/substrate/primitives/core/src/sr25519.rs#L714

  std::vector<uint8_t> seed;
  base::HexStringToBytes(
      "9d61b19deffd5a60ba844af492ec2cc44449c5697b326919703bac031cae7f60",
      &seed);

  auto keypair = HDKeySr25519::GenerateFromSeed(
      base::span<const uint8_t, kSr25519SeedSize>(seed));

  constexpr const char* kExpectedPubKey =
      "44A996BEB1EEF7BDCAB976AB6D2CA26104834164ECF28FB375600576FCC6EB0F";

  EXPECT_EQ(base::HexEncode(keypair.GetPublicKey()), kExpectedPubKey);
}

TEST(HDKeySr25519, DeterministicSignatures) {
  auto keypair = HDKeySr25519::GenerateFromSeed(kSchnorrkelSeed);

  auto message = base::byte_span_from_cstring("hello, world!");

  auto sig1 = keypair.SignMessage(message);
  auto sig2 = keypair.SignMessage(message);

  EXPECT_NE(base::HexEncodeLower(sig1), base::HexEncodeLower(sig2));
  EXPECT_TRUE(keypair.VerifyMessage(sig1, message));
  EXPECT_TRUE(keypair.VerifyMessage(sig2, message));

  keypair.UseMockRngForTesting();

  sig1 = keypair.SignMessage(message);
  sig2 = keypair.SignMessage(message);

  const char expected_sig[] =
      R"(b45c968fd66bbb503bedd3a4735fca241a7867cb9b07989dd36bf4837cad4377a8b1fe8135a97ab85ccb5ff7bb0381890d4b78298fec1ac8ffc086387071688e)";

  EXPECT_EQ(base::HexEncodeLower(sig1), expected_sig);
  EXPECT_EQ(base::HexEncodeLower(sig2), expected_sig);

  EXPECT_TRUE(keypair.VerifyMessage(sig1, message));
  EXPECT_TRUE(keypair.VerifyMessage(sig2, message));
}

}  // namespace brave_wallet
