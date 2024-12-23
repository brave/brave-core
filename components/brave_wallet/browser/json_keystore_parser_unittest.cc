/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/json_keystore_parser.h"

#include "base/test/values_test_util.h"
#include "brave/components/brave_wallet/browser/internal/hd_key.h"
#include "brave/components/brave_wallet/common/eth_address.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::test::ParseJsonDict;

namespace brave_wallet {

namespace {

std::string GetHexAddr(const HDKey* key) {
  const std::vector<uint8_t> public_key = key->GetUncompressedPublicKey();
  // trim the header byte 0x04
  const std::vector<uint8_t> pubkey_no_header(public_key.begin() + 1,
                                              public_key.end());
  EthAddress addr = EthAddress::FromPublicKey(pubkey_no_header);
  return addr.ToHex();
}

}  // namespace

TEST(JsonKeystoreParser, DecryptPrivateKeyFromJsonKeystore) {
  const std::string json(
      R"({
          "address":"b14ab53e38da1c172f877dbc6d65e4a1b0474c3c",
          "crypto" : {
              "cipher" : "aes-128-ctr",
              "cipherparams" : {
                  "iv" : "cecacd85e9cb89788b5aab2f93361233"
              },
              "ciphertext" : "c52682025b1e5d5c06b816791921dbf439afe7a053abb9fac19f38a57499652c",
              "kdf" : "scrypt",
              "kdfparams" : {
                  "dklen" : 32,
                  "n" : 262144,
                  "p" : 1,
                  "r" : 8,
                  "salt" : "dc9e4a98886738bd8aae134a1f89aaa5a502c3fbd10e336136d4d5fe47448ad6"
              },
              "mac" : "27b98c8676dc6619d077453b38db645a4c7c17a3e686ee5adaf53c11ac1b890e"
          },
          "id" : "7e59dc02-8d42-409d-b29a-a8a0f862cc81",
          "version" : 3
      })");
  auto private_key =
      DecryptPrivateKeyFromJsonKeystore("testtest", ParseJsonDict(json));
  ASSERT_TRUE(private_key);

  auto hd_key = HDKey::GenerateFromPrivateKey(
      *base::span(*private_key).to_fixed_extent<32>());
  EXPECT_EQ(GetHexAddr(hd_key.get()),
            "0xb14ab53e38da1c172f877dbc6d65e4a1b0474c3c");
  EXPECT_EQ(HexEncodeLower(*private_key),
            "efca4cdd31923b50f4214af5d2ae10e7ac45a5019e9431cc195482d707485378");

  // wrong password
  EXPECT_FALSE(
      DecryptPrivateKeyFromJsonKeystore("brave1234", ParseJsonDict(json)));
  EXPECT_FALSE(DecryptPrivateKeyFromJsonKeystore("", ParseJsonDict(json)));

  // |N| > 2^(128 * |r| / 8)
  const std::string invalid_r(
      R"({
        "crypto" : {
            "cipher" : "aes-128-ctr",
            "cipherparams" : {
                "iv" : "83dbcc02d8ccb40e466191a123791e0e"
            },
            "ciphertext" : "d172bf743a674da9cdad04534d56926ef8358534d458fffccd4e6ad2fbde479c",
            "kdf" : "scrypt",
            "kdfparams" : {
                "dklen" : 32,
                "n" : 262144,
                "p" : 8,
                "r" : 1,
                "salt" : "ab0c7876052600dd703518d6fc3fe8984592145b591fc8fb5c6d43190334ba19"
            },
            "mac" : "2103ac29920d71da29f15d75b4a16dbe95cfd7ff8faea1056c33131d846e3097"
        },
        "id" : "3198bc9c-6672-5ab3-d995-4942343ae5b6",
        "version" : 3
      })");

  EXPECT_FALSE(
      DecryptPrivateKeyFromJsonKeystore("testtest", ParseJsonDict(invalid_r)));

  const std::string pbkdf2_json(
      R"({
        "address":"b14ab53e38da1c172f877dbc6d65e4a1b0474c3c",
        "crypto" : {
            "cipher" : "aes-128-ctr",
            "cipherparams" : {
                "iv" : "cecacd85e9cb89788b5aab2f93361233"
            },
            "ciphertext" : "01ee7f1a3c8d187ea244c92eea9e332ab0bb2b4c902d89bdd71f80dc384da1be",
            "kdf" : "pbkdf2",
            "kdfparams" : {
                "c" : 262144,
                "dklen" : 32,
                "prf" : "hmac-sha256",
                "salt" : "dc9e4a98886738bd8aae134a1f89aaa5a502c3fbd10e336136d4d5fe47448ad6"
            },
            "mac" : "0c02cd0badfebd5e783e0cf41448f84086a96365fc3456716c33641a86ebc7cc"
        },
        "id" : "7e59dc02-8d42-409d-b29a-a8a0f862cc81",
        "version" : 3
      })");

  auto private_key2 =
      DecryptPrivateKeyFromJsonKeystore("testtest", ParseJsonDict(pbkdf2_json));
  auto hd_key2 = HDKey::GenerateFromPrivateKey(
      *base::span(*private_key2).to_fixed_extent<32>());
  ASSERT_TRUE(hd_key2);
  EXPECT_EQ(GetHexAddr(hd_key2.get()),
            "0xb14ab53e38da1c172f877dbc6d65e4a1b0474c3c");
}

}  // namespace brave_wallet
