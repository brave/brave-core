/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/filecoin_keyring.h"

#include "base/base64.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/bls/buildflags.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

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
#endif
}  // namespace brave_wallet
