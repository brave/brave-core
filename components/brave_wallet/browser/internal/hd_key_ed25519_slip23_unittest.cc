/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/internal/hd_key_ed25519_slip23.h"

#include <array>
#include <string>
#include <vector>

#include "base/containers/span.h"
#include "base/files/file_util.h"
#include "base/numerics/checked_math.h"
#include "base/path_service.h"
#include "base/rand_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/values_test_util.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/internal/hd_key_common.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "crypto/hash.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/boringssl/src/include/openssl/curve25519.h"

namespace brave_wallet {

namespace {

bool VerifySignature(const HDKeyEd25519Slip23& key,
                     base::span<const uint8_t> msg,
                     base::span<const uint8_t, kEd25519SignatureSize> sig) {
  return !!ED25519_verify(msg.data(), msg.size(), sig.data(),
                          key.GetPublicKeyAsSpan().data());
}

}  // namespace

// https://github.com/input-output-hk/cardano-js-sdk/blob/edb73add3841d4f317bff7392c0b7112cfd265d5/packages/crypto/test/ed25519e/Ed25519TestVectors.ts#L197-L239
TEST(HDKeyEd25519Slip23UnitTest, TestVector1) {
  std::array<uint8_t, 32> entropy = {};
  base::HexStringToSpan(
      "f07e8b397c93a16c06f83c8f0c1a1866477c6090926445fc0cb1201228ace6e9",
      entropy);

  auto message = crypto::hash::Sha512(base::byte_span_from_cstring("abc"));
  EXPECT_EQ(base::HexEncodeLower(message),
            "ddaf35a193617abacc417349ae204131"
            "12e6fa4e89a97ea20a9eeee64b55d39a"
            "2192992a274fc1a836ba3c23a3feebbd"
            "454d4423643ce80e2a9ac94fa54ca49f");

  auto master_key =
      HDKeyEd25519Slip23::GenerateMasterKeyFromBip39Entropy(entropy);
  EXPECT_EQ(base::HexEncodeLower(master_key->GetPublicKeyAsSpan()),
            "311f8914b8934efbe7cbb8cc4745853d"
            "e12e8ea402df6f9f69b18d2792c6bed8");
  auto signature = *master_key->Sign(message);
  EXPECT_EQ(base::HexEncodeLower(signature),
            "843aa4353184193bdf01aab7f636ac53"
            "f86746dd97a2a2e01fe7923c37bfec40"
            "b68a73881a26ba57dc974abc1123d086"
            "6b542a5447e03677134a8f4e1db2bc0c");
  EXPECT_TRUE(VerifySignature(*master_key, message, signature));

  auto derived = master_key->DeriveChild(DerivationIndex::Hardened(1852))
                     ->DeriveChild(DerivationIndex::Hardened(1815))
                     ->DeriveChild(DerivationIndex::Hardened(0));

  EXPECT_EQ(base::HexEncodeLower(derived->GetPublicKeyAsSpan()),
            "a79619cd18f11202741213ab003dd40b"
            "ffb2a31e8ad1bc5aab6f02be3c8aa921");
}

// https://github.com/input-output-hk/cardano-js-sdk/blob/edb73add3841d4f317bff7392c0b7112cfd265d5/packages/crypto/test/ed25519e/Ed25519TestVectors.ts#L281-L323
TEST(HDKeyEd25519Slip23UnitTest, TestVector2) {
  std::array<uint8_t, 32> entropy = {};
  base::HexStringToSpan(
      "be9ffd296c0ccabadf51c6fbb904995b182d6ac84181c08d8b016ab1eefd78ce",
      entropy);

  auto message = crypto::hash::Sha512(base::byte_span_from_cstring("abc"));
  EXPECT_EQ(base::HexEncodeLower(message),
            "ddaf35a193617abacc417349ae204131"
            "12e6fa4e89a97ea20a9eeee64b55d39a"
            "2192992a274fc1a836ba3c23a3feebbd"
            "454d4423643ce80e2a9ac94fa54ca49f");

  auto master_key =
      HDKeyEd25519Slip23::GenerateMasterKeyFromBip39Entropy(entropy);
  EXPECT_EQ(base::HexEncodeLower(master_key->GetPublicKeyAsSpan()),
            "6fd8d9c696b01525cc45f15583fc9447"
            "c66e1c71fd1a11c8885368404cd0a4ab");
  auto signature = *master_key->Sign(message);
  EXPECT_EQ(base::HexEncodeLower(signature),
            "f363d78e0a315ae1fc0ceb6b8efdd163"
            "1a3a2ce16f6cf43f596ff92c4a7b2926"
            "39c6e352cc24efcf80ccea39cbdb7ec9"
            "a02f4a5b332afc2de7f7a2e65e67780e");
  EXPECT_TRUE(VerifySignature(*master_key, message, signature));

  auto derived = master_key->DeriveChild(DerivationIndex::Normal(1852))
                     ->DeriveChild(DerivationIndex::Normal(1815))
                     ->DeriveChild(DerivationIndex::Normal(0));

  EXPECT_EQ(base::HexEncodeLower(derived->GetPublicKeyAsSpan()),
            "b857a8cd1dbbfed1824359d9d9e58bc8"
            "ffb9f66812b404f4c6ffc315629835bf");
}

TEST(HDKeyEd25519Slip23UnitTest, RandomEntropyDeepDerivation) {
  std::array<uint8_t, 32> entropy = {};
  base::RandBytes(entropy);
  SCOPED_TRACE(testing::Message() << base::HexEncodeLower(entropy));

  auto key = HDKeyEd25519Slip23::GenerateMasterKeyFromBip39Entropy(entropy);
  ASSERT_TRUE(key);

  std::string path;
  // Do random derivations from a master key generated from random entropy.
  // Should never fail.
  for (auto i = 0u; i < 1000; ++i) {
    auto index = DerivationIndex::FromRawValueForTesting(base::RandUint64() &
                                                         0xffffffff);
    path.append(" " + base::NumberToString(*index.GetValue()));
    SCOPED_TRACE(testing::Message() << path);

    key = key->DeriveChild(index);
    ASSERT_TRUE(key);

    auto message = base::byte_span_from_cstring("message");
    auto signature = key->Sign(message);
    ASSERT_TRUE(signature);
    ASSERT_TRUE(VerifySignature(*key, message, *signature));
  }
}

TEST(HDKeyEd25519Slip23UnitTest, Errors) {
  std::vector<uint8_t> entropy;
  EXPECT_TRUE(
      base::HexStringToBytes("000102030405060708090a0b0c0d0e0f", &entropy));
  auto master_key =
      HDKeyEd25519Slip23::GenerateMasterKeyFromBip39Entropy(entropy);

  // index is too big for hardened index
  auto child = master_key->DeriveChild(DerivationIndex::Hardened(0x80000000));
  EXPECT_FALSE(child);
}

TEST(HDKeyEd25519Slip23UnitTest, SignAndVerify) {
  std::vector<uint8_t> entropy;
  EXPECT_TRUE(
      base::HexStringToBytes("000102030405060708090a0b0c0d0e0f", &entropy));
  auto key = HDKeyEd25519Slip23::GenerateMasterKeyFromBip39Entropy(entropy);
  const std::vector<uint8_t> msg_a(32, 0x00);
  const std::vector<uint8_t> msg_b(32, 0x08);
  const auto sig_a = key->Sign(msg_a);
  const auto sig_b = key->Sign(msg_b);

  EXPECT_EQ(sig_a->size(), 64u);
  EXPECT_EQ(sig_b->size(), 64u);

  EXPECT_TRUE(VerifySignature(*key, msg_a, *sig_a));
  EXPECT_TRUE(VerifySignature(*key, msg_b, *sig_b));

  // wrong signature
  EXPECT_FALSE(VerifySignature(*key, msg_a, *sig_b));
  EXPECT_FALSE(VerifySignature(*key, msg_b, *sig_a));
}

TEST(HDKeyEd25519Slip23UnitTest, CustomED25519Sign) {
  auto message = base::byte_span_from_cstring("brave");

  for (auto i = 0u; i < 1000; ++i) {
    std::array<uint8_t, ED25519_PUBLIC_KEY_LEN> public_key_not_used;
    std::array<uint8_t, ED25519_PRIVATE_KEY_LEN> private_key;
    ED25519_keypair(public_key_not_used.data(), private_key.data());

    std::array<uint8_t, ED25519_SIGNATURE_LEN> signature1;
    ED25519_sign(signature1.data(), message.data(), message.size(),
                 private_key.data());

    auto private_key_hash =
        crypto::hash::Sha512(base::span(private_key).first<32>());

    private_key_hash[0] &= 248;
    private_key_hash[31] &= 63;
    private_key_hash[31] |= 64;

    auto scalar = base::span(private_key_hash).first<32>();
    auto prefix = base::span(private_key_hash).last<32>();
    auto public_key = base::span(private_key).last<32>();

    std::array<uint8_t, ED25519_SIGNATURE_LEN> signature2;
    ASSERT_TRUE(ED25519_sign_with_scalar_and_prefix(
        signature2.data(), message.data(), message.size(), scalar.data(),
        prefix.data(), public_key.data()));

    ASSERT_EQ(signature1, signature2);
  }
}

TEST(HDKeyEd25519Slip23UnitTest, CardanoSdkCryptoSlip23) {
  std::string file_contents;

  ASSERT_TRUE(
      base::ReadFileToString(BraveWalletComponentsTestDataFolder()
                                 .AppendASCII("cardano")
                                 .AppendASCII("cardano_sdk_crypto_slip23")
                                 .AppendASCII("test_vectors.json"),
                             &file_contents));

  auto test_items = base::test::ParseJsonList(file_contents);
  ASSERT_EQ(test_items.size(), 1000u);

  for (auto& test : test_items) {
    auto& test_dict = test.GetDict();
    auto* name = test_dict.FindString("test");
    SCOPED_TRACE(testing::Message() << name);

    std::vector<uint8_t> entropy;
    base::HexStringToBytes(*test_dict.FindString("entropy"), &entropy);

    auto key = HDKeyEd25519Slip23::GenerateMasterKeyFromBip39Entropy(entropy);

    for (auto& index : *test_dict.FindList("path")) {
      key = key->DeriveChild(DerivationIndex::FromRawValueForTesting(
          base::CheckedNumeric<uint32_t>(index.GetDouble()).ValueOrDie()));
      ASSERT_TRUE(key);
    }

    // Reference implementation encodes pubkey as `pubkey|chain_code`.
    ASSERT_EQ(*test_dict.FindString("pubkey"),
              base::HexEncodeLower(key->GetPublicKeyAsSpan()) +
                  base::HexEncodeLower(key->GetChainCodeAsSpanForTesting()));

    // Reference implementation encodes private key as
    // `scalar|prefix|chain_code`.
    ASSERT_EQ(*test_dict.FindString("privatekey"),
              base::HexEncodeLower(key->GetScalarAsSpanForTesting()) +
                  base::HexEncodeLower(key->GetPrefixAsSpanForTesting()) +
                  base::HexEncodeLower(key->GetChainCodeAsSpanForTesting()));

    ASSERT_EQ(*test_dict.FindString("signature"),
              base::HexEncodeLower(
                  *key->Sign(base::byte_span_from_cstring("message"))));
  }
}

}  // namespace brave_wallet
