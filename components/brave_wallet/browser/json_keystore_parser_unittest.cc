/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/json_keystore_parser.h"

#include <string>

#include "base/test/values_test_util.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/internal/hd_key.h"
#include "brave/components/brave_wallet/common/eth_address.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::test::ParseJsonDict;

namespace brave_wallet {

namespace {

std::string GetHexAddr(const HDKey* key) {
  const std::vector<uint8_t> public_key = key->GetUncompressedPublicKey();
  // trim the header byte 0x04
  const std::vector<uint8_t> pubkey_no_header(public_key.begin() + 1,
                                              public_key.end());
  auto addr = EthAddress::FromPublicKey(
      base::span(pubkey_no_header).to_fixed_extent<64>().value());
  return addr.ToHex();
}

// Valid v3 pbkdf2 keystore. Used as a base for negative cases because pbkdf2 is
// much cheaper than the scrypt fixture.
constexpr char kPbkdf2Keystore[] =
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
      })";

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
  EXPECT_EQ(base::HexEncodeLower(*private_key),
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

TEST(JsonKeystoreParser, Pbkdf2BaseKeystoreIsValid) {
  // Sanity check that kPbkdf2Keystore, the base fixture of the Rejects* tests,
  // decrypts.
  auto private_key = DecryptPrivateKeyFromJsonKeystore(
      "testtest", ParseJsonDict(kPbkdf2Keystore));
  ASSERT_TRUE(private_key);
  EXPECT_EQ(private_key->size(), 32u);
}

TEST(JsonKeystoreParser, RejectsUnsupportedCipher) {
  // MAC is computed over derived_key[16:32] || ciphertext and ignores the
  // cipher name, so it stays valid here.
  auto dict = ParseJsonDict(kPbkdf2Keystore);
  dict.SetByDottedPath("crypto.cipher", "aes-128-cbc");
  EXPECT_FALSE(DecryptPrivateKeyFromJsonKeystore("testtest", dict));
}

TEST(JsonKeystoreParser, RejectsUnsupportedVersion) {
  auto dict = ParseJsonDict(kPbkdf2Keystore);
  dict.Set("version", 4);
  EXPECT_FALSE(DecryptPrivateKeyFromJsonKeystore("testtest", dict));

  dict = ParseJsonDict(kPbkdf2Keystore);
  dict.Set("version", 2);
  EXPECT_FALSE(DecryptPrivateKeyFromJsonKeystore("testtest", dict));

  dict = ParseJsonDict(kPbkdf2Keystore);
  ASSERT_TRUE(dict.Remove("version"));
  EXPECT_FALSE(DecryptPrivateKeyFromJsonKeystore("testtest", dict));
}

TEST(JsonKeystoreParser, RejectsUnsupportedKdfParams) {
  // dklen other than 32.
  auto dict = ParseJsonDict(kPbkdf2Keystore);
  dict.SetByDottedPath("crypto.kdfparams.dklen", 16);
  EXPECT_FALSE(DecryptPrivateKeyFromJsonKeystore("testtest", dict));

  // Unknown kdf.
  dict = ParseJsonDict(kPbkdf2Keystore);
  dict.SetByDottedPath("crypto.kdf", "argon2");
  EXPECT_FALSE(DecryptPrivateKeyFromJsonKeystore("testtest", dict));

  // Unsupported pbkdf2 prf.
  dict = ParseJsonDict(kPbkdf2Keystore);
  dict.SetByDottedPath("crypto.kdfparams.prf", "hmac-sha512");
  EXPECT_FALSE(DecryptPrivateKeyFromJsonKeystore("testtest", dict));

  // Iteration count which does not fit into uint32_t.
  dict = ParseJsonDict(kPbkdf2Keystore);
  dict.SetByDottedPath("crypto.kdfparams.c", -1);
  EXPECT_FALSE(DecryptPrivateKeyFromJsonKeystore("testtest", dict));

  // Salt and iv which are not valid hex of expected size.
  dict = ParseJsonDict(kPbkdf2Keystore);
  dict.SetByDottedPath("crypto.kdfparams.salt", "zz");
  EXPECT_FALSE(DecryptPrivateKeyFromJsonKeystore("testtest", dict));

  dict = ParseJsonDict(kPbkdf2Keystore);
  dict.SetByDottedPath("crypto.cipherparams.iv", "cecacd85");
  EXPECT_FALSE(DecryptPrivateKeyFromJsonKeystore("testtest", dict));
}

TEST(JsonKeystoreParser, RejectsMissingFields) {
  for (const char* path :
       {"crypto", "crypto.kdf", "crypto.kdfparams", "crypto.kdfparams.dklen",
        "crypto.kdfparams.salt", "crypto.kdfparams.c", "crypto.kdfparams.prf",
        "crypto.mac", "crypto.ciphertext", "crypto.cipher",
        "crypto.cipherparams", "crypto.cipherparams.iv"}) {
    SCOPED_TRACE(path);
    auto dict = ParseJsonDict(kPbkdf2Keystore);
    ASSERT_TRUE(dict.RemoveByDottedPath(path));
    EXPECT_FALSE(DecryptPrivateKeyFromJsonKeystore("testtest", dict));
  }
}

TEST(JsonKeystoreParser, RejectsMissingScryptParams) {
  for (const char* missing : {"n", "r", "p"}) {
    SCOPED_TRACE(missing);
    auto dict = ParseJsonDict(kPbkdf2Keystore);
    dict.SetByDottedPath("crypto.kdf", "scrypt");
    dict.SetByDottedPath("crypto.kdfparams.n", 16);
    dict.SetByDottedPath("crypto.kdfparams.r", 1);
    dict.SetByDottedPath("crypto.kdfparams.p", 1);
    ASSERT_TRUE(dict.FindDictByDottedPath("crypto.kdfparams")->Remove(missing));
    EXPECT_FALSE(DecryptPrivateKeyFromJsonKeystore("testtest", dict));
  }
}

}  // namespace brave_wallet
