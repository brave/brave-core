/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/filecoin_keyring.h"

#include "base/base64.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/bls/buildflags.h"
#include "testing/gtest/include/gtest/gtest.h"
#if BUILDFLAG(ENABLE_RUST_BLS)
#include "brave/components/bls/rs/src/lib.rs.h"
#endif

namespace brave_wallet {

namespace {
std::string GetPublicKey(const std::string& private_key_hex) {
  std::vector<uint8_t> private_key;
  base::HexStringToBytes(private_key_hex, &private_key);
  std::array<uint8_t, 32> payload;
  std::copy_n(private_key.begin(), 32, payload.begin());
  auto result = bls::fil_private_key_public_key(payload);
  std::vector<uint8_t> public_key(result.begin(), result.end());
  return base::HexEncode(public_key);
}
}  // namespace

TEST(FilecoinKeyring, ImportFilecoinSECP) {
  std::string private_key_base64 =
      "rQG5jnbc+y64fckG+T0EHVwpLBmW9IgAT7U990HXcGk=";
  std::string input_key;
  ASSERT_TRUE(base::Base64Decode(private_key_base64, &input_key));
  ASSERT_FALSE(input_key.empty());
  std::vector<uint8_t> private_key(input_key.begin(), input_key.end());

  FilecoinKeyring keyring;
  auto address = keyring.ImportFilecoinSECP256K1Account(
      private_key, mojom::kFilecoinTestnet);
  EXPECT_EQ(address, "t1lqarsh4nkg545ilaoqdsbtj4uofplt6sto26ziy");
  EXPECT_EQ(keyring.GetImportedAccountsNumber(), size_t(1));
}
#if BUILDFLAG(ENABLE_RUST_BLS)
TEST(FilecoinKeyring, ImportFilecoinBLS) {
  std::string private_key_hex =
      "6a4b3d3f3ccb3676e34e16bc07a9371dede3a037def6114e79e51705f823723f";
  std::vector<uint8_t> private_key;
  ASSERT_TRUE(base::HexStringToBytes(private_key_hex, &private_key));

  FilecoinKeyring keyring;
  std::string address =
      keyring.ImportFilecoinBLSAccount(private_key, mojom::kFilecoinTestnet);
  EXPECT_EQ(address,
            "t3wv3u6pmfi3j6pf3fhjkch372pkyg2tgtlb3jpu3eo6mnt7ttsft6x2xr54ct7fl2"
            "oz4o4tpa4mvigcrayh4a");
  EXPECT_EQ(keyring.GetImportedAccountsNumber(), size_t(1));

  // empty private key
  ASSERT_TRUE(
      keyring.ImportFilecoinBLSAccount({}, mojom::kFilecoinTestnet).empty());

  // broken private key
  private_key_hex = "6a4b3d3f3ccb3676e34e16bc07a937";
  std::vector<uint8_t> broken_private_key;
  ASSERT_TRUE(base::HexStringToBytes(private_key_hex, &broken_private_key));
  ASSERT_TRUE(
      keyring
          .ImportFilecoinBLSAccount(broken_private_key, mojom::kFilecoinTestnet)
          .empty());

  std::vector<uint8_t> zero_private_key(32, 0);
  EXPECT_EQ(keyring.ImportFilecoinBLSAccount(zero_private_key,
                                             mojom::kFilecoinTestnet),
            "t3yaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
            "aaaaaaaaaaaaby2smx7a");
  std::vector<uint8_t> ff_private_key(32, 255);
  ASSERT_TRUE(
      keyring.ImportFilecoinBLSAccount(ff_private_key, mojom::kFilecoinTestnet)
          .empty());
}

TEST(FilecoinKeyring, fil_private_key_public_key) {
  std::string private_key_hex =
      "6a4b3d3f3ccb3676e34e16bc07a9371dede3a037def6114e79e51705f823723f";
  EXPECT_EQ(GetPublicKey(private_key_hex),
            "B5774F3D8546D3E797653A5423EFFA7AB06D4CD3587697D3647798D9FE739167EB"
            "EAF1EF053F957A7678EE4DE0E32A83");

  std::vector<uint8_t> ff_private_key(32, 255);
  std::array<uint8_t, 32> payload;
  std::copy_n(ff_private_key.begin(), 32, payload.begin());
  auto result = bls::fil_private_key_public_key(payload);
  std::vector<uint8_t> public_key(result.begin(), result.end());
  ASSERT_TRUE(std::all_of(public_key.begin(), public_key.end(),
                          [](int i) { return i == 0; }));
}
#endif
}  // namespace brave_wallet
