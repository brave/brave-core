/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/filecoin_keyring.h"

#include "base/base64.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/filecoin/rs/src/lib.rs.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::ElementsAre;

namespace brave_wallet {

namespace {
std::vector<uint8_t> GetPublicKey(const std::string& private_key_hex) {
  std::vector<uint8_t> private_key;
  base::HexStringToBytes(private_key_hex, &private_key);
  auto result = filecoin::bls_private_key_to_public_key(
      rust::Slice<const uint8_t>{private_key.data(), private_key.size()});
  std::vector<uint8_t> public_key(result.begin(), result.end());
  return public_key;
}
}  // namespace

TEST(FilecoinKeyring, DecodeImportPayload) {
  std::string payload_hex;
  std::vector<uint8_t> private_key;
  mojom::FilecoinAddressProtocol protocol;
  // zero key
  ASSERT_FALSE(
      FilecoinKeyring::DecodeImportPayload(payload_hex, nullptr, &protocol));
  // zero protocol
  ASSERT_FALSE(
      FilecoinKeyring::DecodeImportPayload(payload_hex, &private_key, nullptr));
  EXPECT_TRUE(private_key.empty());

  ASSERT_FALSE(
      FilecoinKeyring::DecodeImportPayload("", &private_key, &protocol));
  EXPECT_TRUE(private_key.empty());

  // broken json
  ASSERT_FALSE(FilecoinKeyring::DecodeImportPayload(
      "7b2254797065223a22626c73222c22507269766174654b6579223a2270536e7752332f38"
      "55616b53516f777858742b345a75393257586d424d526e74716d6448696136724853453d"
      "22",
      &private_key, &protocol));
  EXPECT_TRUE(private_key.empty());

  // no type in json
  ASSERT_FALSE(FilecoinKeyring::DecodeImportPayload(
      "7b22507269766174654b6579223a2270536e7752332f3855616b53516f777858742b345a"
      "75393257586d424d526e74716d6448696136724853453d22207d",
      &private_key, &protocol));
  EXPECT_TRUE(private_key.empty());

  // no private key in json
  ASSERT_FALSE(FilecoinKeyring::DecodeImportPayload(
      "7b2254797065223a22626c73227d", &private_key, &protocol));
  EXPECT_TRUE(private_key.empty());

  // type empty
  ASSERT_FALSE(FilecoinKeyring::DecodeImportPayload(
      "7b2254797065223a22222c22507269766174654b6579223a2270536e7752332f3855616b"
      "53516f777858742b345a75393257586d424d526e74716d6448696136724853453d227d",
      &private_key, &protocol));
  EXPECT_TRUE(private_key.empty());

  // private key empty
  ASSERT_FALSE(FilecoinKeyring::DecodeImportPayload(
      "7b2254797065223a22626c73222c22507269766174654b6579223a22227d",
      &private_key, &protocol));
  EXPECT_TRUE(private_key.empty());

  // private key with broken encoding
  ASSERT_FALSE(FilecoinKeyring::DecodeImportPayload(
      "7b2254797065223a22626c73222c22507269766174654b6579223a227053227d",
      &private_key, &protocol));
  EXPECT_TRUE(private_key.empty());
  // valid payload
  payload_hex =
      "7b2254797065223a22626c73222c22507269766174654b6579223a2270536e7752332f38"
      "55616b53516f777858742b345a75393257586d424d526e74716d6448696136724853453d"
      "227d";
  ASSERT_TRUE(FilecoinKeyring::DecodeImportPayload(payload_hex, &private_key,
                                                   &protocol));
  EXPECT_EQ(base::Base64Encode(private_key),
            "pSnwR3/8UakSQowxXt+4Zu92WXmBMRntqmdHia6rHSE=");
  EXPECT_EQ(protocol, mojom::FilecoinAddressProtocol::BLS);
}

TEST(FilecoinKeyring, ImportFilecoinSECP) {
  std::string private_key_base64 =
      "rQG5jnbc+y64fckG+T0EHVwpLBmW9IgAT7U990HXcGk=";
  std::string input_key;
  ASSERT_TRUE(base::Base64Decode(private_key_base64, &input_key));
  ASSERT_FALSE(input_key.empty());
  std::vector<uint8_t> private_key(input_key.begin(), input_key.end());

  FilecoinKeyring keyring(*MnemonicToSeed(kMnemonicDivideCruise),
                          brave_wallet::mojom::kFilecoinTestnet);
  auto address = keyring.ImportFilecoinAccount(
      private_key, mojom::FilecoinAddressProtocol::SECP256K1);
  EXPECT_THAT(keyring.GetImportedAccountsForTesting(),
              ElementsAre("t1lqarsh4nkg545ilaoqdsbtj4uofplt6sto26ziy"));
}

TEST(FilecoinKeyring, ImportFilecoinBLS) {
  std::string private_key_hex =
      "7b2254797065223a22626c73222c22507269766174654b6579223a2270536e7752332f38"
      "55616b53516f777858742b345a75393257586d424d526e74716d6448696136724853453d"
      "227d";
  std::vector<uint8_t> private_key;
  mojom::FilecoinAddressProtocol protocol;
  ASSERT_TRUE(FilecoinKeyring::DecodeImportPayload(private_key_hex,
                                                   &private_key, &protocol));
  EXPECT_EQ(protocol, mojom::FilecoinAddressProtocol::BLS);
  FilecoinKeyring keyring(*MnemonicToSeed(kMnemonicDivideCruise),
                          brave_wallet::mojom::kFilecoinTestnet);
  std::string address = keyring.ImportFilecoinAccount(private_key, protocol);
  EXPECT_EQ(address,
            "t3wwtato54ee5aod7j5uv2n75jpyn4hpwx3f2kx5cijtoxgytiul2dczrak3ghlbt5"
            "zjnj574y3snhcb5bthva");
  EXPECT_THAT(
      keyring.GetImportedAccountsForTesting(),
      ElementsAre(
          "t3wwtato54ee5aod7j5uv2n75jpyn4hpwx3f2kx5cijtoxgytiul2dczrak3ghlbt5"
          "zjnj574y3snhcb5bthva"));

  // empty private key
  ASSERT_TRUE(
      keyring.ImportFilecoinAccount({}, mojom::FilecoinAddressProtocol::BLS)
          .empty());

  // broken private key
  private_key_hex = "6a4b3d3f3ccb3676e34e16bc07a937";
  std::vector<uint8_t> broken_private_key;
  ASSERT_TRUE(base::HexStringToBytes(private_key_hex, &broken_private_key));
  ASSERT_TRUE(keyring
                  .ImportFilecoinAccount(broken_private_key,
                                         mojom::FilecoinAddressProtocol::BLS)
                  .empty());

  std::vector<uint8_t> zero_private_key(32, 0);
  EXPECT_EQ(keyring.ImportFilecoinAccount(zero_private_key,
                                          mojom::FilecoinAddressProtocol::BLS),
            "t3yaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
            "aaaaaaaaaaaaby2smx7a");
  std::vector<uint8_t> ff_private_key(32, 255);
  ASSERT_TRUE(keyring
                  .ImportFilecoinAccount(ff_private_key,
                                         mojom::FilecoinAddressProtocol::BLS)
                  .empty());
}

TEST(FilecoinKeyring, fil_private_key_public_key) {
  std::string private_key_hex =
      "6a4b3d3f3ccb3676e34e16bc07a9371dede3a037def6114e79e51705f823723f";
  EXPECT_EQ(base::HexEncode(GetPublicKey(private_key_hex)),
            "B5774F3D8546D3E797653A5423EFFA7AB06D4CD3587697D3647798D9FE739167EB"
            "EAF1EF053F957A7678EE4DE0E32A83");

  std::vector<uint8_t> ff_private_key(32, 255);
  auto zero_key = GetPublicKey(base::HexEncode(ff_private_key));
  ASSERT_TRUE(std::all_of(zero_key.begin(), zero_key.end(),
                          [](int i) { return i == 0; }));
}

TEST(FilecoinKeyring, SignTransaction) {
  FilecoinKeyring keyring(*MnemonicToSeed(kMnemonicDivideCruise),
                          brave_wallet::mojom::kFilecoinTestnet);
  const std::string address = "t1lqarsh4nkg545ilaoqdsbtj4uofplt6sto26ziy";
  EXPECT_FALSE(keyring.SignTransaction(address, nullptr));

  auto transaction = FilTransaction::FromTxData(
      false,
      mojom::FilTxData::New("1", "2", "3", "4", "5",
                            "t1h5tg3bhp5r56uzgjae2373znti6ygq4agkx4hzq", "6"));
  EXPECT_FALSE(keyring.SignTransaction(address, &transaction.value()));

  std::string private_key_base64 =
      "rQG5jnbc+y64fckG+T0EHVwpLBmW9IgAT7U990HXcGk=";
  std::string input_key;
  ASSERT_TRUE(base::Base64Decode(private_key_base64, &input_key));
  ASSERT_FALSE(input_key.empty());
  std::vector<uint8_t> private_key(input_key.begin(), input_key.end());

  auto imported_address = keyring.ImportFilecoinAccount(
      private_key, mojom::FilecoinAddressProtocol::SECP256K1);
  EXPECT_EQ(address, imported_address);
  EXPECT_TRUE(keyring.SignTransaction(address, &transaction.value()));
}

}  // namespace brave_wallet
