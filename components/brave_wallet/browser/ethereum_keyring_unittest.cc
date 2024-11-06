/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifdef UNSAFE_BUFFERS_BUILD
// TODO(https://github.com/brave/brave-browser/issues/41661): Remove this and
// convert code to safer constructs.
#pragma allow_unsafe_buffers
#endif

#include "brave/components/brave_wallet/browser/ethereum_keyring.h"

#include <memory>
#include <optional>
#include <utility>

#include "base/base64.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/eth_transaction.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::ElementsAre;

namespace {
constexpr char kMnemonic[] =
    "home various adjust motion canvas stand combine gravity cluster behave "
    "despair dove";
}

namespace brave_wallet {

TEST(EthereumKeyringUnitTest, ConstructRootHDKey) {
  std::vector<uint8_t> seed;
  EXPECT_TRUE(base::HexStringToBytes(
      "13ca6c28d26812f82db27908de0b0b7b18940cc4e9d96ebd7de190f706741489907ef65b"
      "8f9e36c31dc46e81472b6a5e40a4487e725ace445b8203f243fb8958",
      &seed));
  EthereumKeyring keyring(seed);
  EXPECT_EQ(
      keyring.root_.get()->GetPrivateExtendedKey(ExtendedKeyVersion::kXprv),
      "xprvA1YGbmYkUq9KMyPwADQehauc1vG7TSbNLc1dwYbvU7VzyAr7TPhj9VoJJoP2CV5kDmXX"
      "SZvbJ79ieLnD7Pt4rhbuaQjVr2JE3vcDBAvDoUg");
  EXPECT_EQ(
      keyring.root_.get()->GetPublicExtendedKey(ExtendedKeyVersion::kXpub),
      "xpub6EXd1H5eKChcaTUQGEwf4irLZx6bruKDhpwEjw1Y2T2yqyBFzw1yhJ7nA5EeBKozqYKB"
      "8jHxmhe7bEqyBEdPNWyPgCm2aZfs9tbLVYujvL3");
}

TEST(EthereumKeyringUnitTest, EncodePrivateKeyForExport) {
  std::vector<uint8_t> seed;
  EXPECT_TRUE(base::HexStringToBytes(
      "13ca6c28d26812f82db27908de0b0b7b18940cc4e9d96ebd7de190f706741489907ef65b"
      "8f9e36c31dc46e81472b6a5e40a4487e725ace445b8203f243fb8958",
      &seed));
  EthereumKeyring keyring(seed);
  auto address = keyring.AddNewHDAccount()->address;
  EXPECT_EQ(keyring.EncodePrivateKeyForExport(address),
            "8140cea58e3bebd6174dbc589a7f70e049556233d32e44969d62e51dd0d1189a");
}

TEST(EthereumKeyringUnitTest, Accounts) {
  std::vector<uint8_t> seed;
  EXPECT_TRUE(base::HexStringToBytes(
      "13ca6c28d26812f82db27908de0b0b7b18940cc4e9d96ebd7de190f706741489907ef65b"
      "8f9e36c31dc46e81472b6a5e40a4487e725ace445b8203f243fb8958",
      &seed));
  EthereumKeyring keyring(seed);
  keyring.AddNewHDAccount();
  EXPECT_THAT(keyring.GetHDAccountsForTesting(),
              ElementsAre("0x2166fB4e11D44100112B1124ac593081519cA1ec"));

  keyring.AddNewHDAccount();
  keyring.AddNewHDAccount();
  EXPECT_THAT(keyring.GetHDAccountsForTesting(),
              ElementsAre("0x2166fB4e11D44100112B1124ac593081519cA1ec",
                          "0x2A22ad45446E8b34Da4da1f4ADd7B1571Ab4e4E7",
                          "0x02e77f0e2fa06F95BDEa79Fad158477723145838"));

  // remove the last account
  keyring.RemoveLastHDAccount();
  EXPECT_THAT(keyring.GetHDAccountsForTesting(),
              ElementsAre("0x2166fB4e11D44100112B1124ac593081519cA1ec",
                          "0x2A22ad45446E8b34Da4da1f4ADd7B1571Ab4e4E7"));

  keyring.AddNewHDAccount();
  EXPECT_THAT(keyring.GetHDAccountsForTesting(),
              ElementsAre("0x2166fB4e11D44100112B1124ac593081519cA1ec",
                          "0x2A22ad45446E8b34Da4da1f4ADd7B1571Ab4e4E7",
                          "0x02e77f0e2fa06F95BDEa79Fad158477723145838"));
}

TEST(EthereumKeyringUnitTest, SignTransaction) {
  // Specific signature check is in eth_transaction_unittest.cc
  EthereumKeyring keyring({});
  EthTransaction tx = *EthTransaction::FromTxData(mojom::TxData::New(
      "0x09", "0x4a817c800", "0x5208",
      "0x3535353535353535353535353535353535353535", "0x0de0b6b3a7640000",
      std::vector<uint8_t>(), false, std::nullopt));
  keyring.SignTransaction("0xDEADBEEFdeadbeefdeadbeefdeadbeefDEADBEEF", &tx, 0);
  EXPECT_FALSE(tx.IsSigned());

  std::vector<uint8_t> seed;
  EXPECT_TRUE(base::HexStringToBytes(
      "13ca6c28d26812f82db27908de0b0b7b18940cc4e9d96ebd7de190f706741489907ef65b"
      "8f9e36c31dc46e81472b6a5e40a4487e725ace445b8203f243fb8958",
      &seed));
  EthereumKeyring keyring2(seed);
  auto address = keyring2.AddNewHDAccount()->address;
  keyring2.SignTransaction(address, &tx, 0);
  EXPECT_TRUE(tx.IsSigned());
}

TEST(EthereumKeyringUnitTest, SignMessage) {
  std::vector<uint8_t> private_key;
  EXPECT_TRUE(base::HexStringToBytes(
      "6969696969696969696969696969696969696969696969696969696969696969",
      &private_key));

  std::unique_ptr<HDKey> key = std::make_unique<HDKey>();
  key->SetPrivateKey(private_key);

  EthereumKeyring keyring(*MnemonicToSeed(kMnemonic));
  keyring.accounts_.push_back(std::move(key));
  auto address = keyring.GetHDAccountsForTesting()[0];
  EXPECT_EQ(address, "0xbE93f9BacBcFFC8ee6663f2647917ed7A20a57BB");

  std::vector<uint8_t> message;
  EXPECT_TRUE(base::HexStringToBytes("deadbeef", &message));
  std::vector<uint8_t> sig = keyring.SignMessage(address, message, 0, false);
  EXPECT_EQ(base::ToLowerASCII(base::HexEncode(sig)),
            "a77440e5c84e5f16ca3636c7af5857c828d2a8f1afbc0a6945d33d4fc45f216e"
            "3eefd69ccc5b3cee000fdaa564d8f1512789af8fe62f2907f5a8c87885b508fa"
            "1b");

  sig = keyring.SignMessage(address, message, 3, false);
  EXPECT_EQ(base::ToLowerASCII(base::HexEncode(sig)),
            "a77440e5c84e5f16ca3636c7af5857c828d2a8f1afbc0a6945d33d4fc45f216e"
            "3eefd69ccc5b3cee000fdaa564d8f1512789af8fe62f2907f5a8c87885b508fa"
            "29");

  sig = keyring.SignMessage(address, message, 300, false);
  EXPECT_EQ(base::ToLowerASCII(base::HexEncode(sig)),
            "a77440e5c84e5f16ca3636c7af5857c828d2a8f1afbc0a6945d33d4fc45f216e"
            "3eefd69ccc5b3cee000fdaa564d8f1512789af8fe62f2907f5a8c87885b508fa"
            "7b");

  EXPECT_TRUE(keyring
                  .SignMessage("0xDEADBEEFdeadbeefdeadbeefdeadbeefDEADBEEF",
                               message, 0, false)
                  .empty());

  // when message is not Keccak hash
  EXPECT_TRUE(keyring.SignMessage(address, message, 0, true).empty());
  message.clear();
  EXPECT_TRUE(base::HexStringToBytes(
      "be609aee343fb3c4b28e1df9e632fca64fcfaede20f02e86244efddf30957bd2",
      &message));
  sig = keyring.SignMessage(address, message, 3, true);
  EXPECT_EQ(base::ToLowerASCII(base::HexEncode(sig)),
            "789c0e9025bbf9410b58c2ca43ea1add3c6cfed66001300b9c102f78022cf6e21b"
            "bf3780d68ff28e72c0ccb4f515b3d527c585abf59bc03531f5047b0357ef3329");
}

TEST(EthereumKeyringUnitTest, ImportedAccounts) {
  const struct {
    const char* key;
    const char* address;
  } private_keys[] = {
      {"d118a12a1e3b595d7d9e5599370df4ddc58d246a3ae4a795597e50eb6a32afb5",
       "0xDc06aE500aD5ebc5972A0D8Ada4733006E905976"},
      {"cca1e9643efc5468789366e4fb682dba57f2e97540981095bc6d9a962309d912",
       "0x6D59205FADC892333cb945AD563e74F83f3dBA95"},
      {"ddc33eef7cc4c5170c3ba4021cc22fd888856cf8bf846f48db6d11d15efcd652",
       "0xeffF78040EdeF86A9be71ce89c74A35C4cd5D2eA"},
      // Used for Sign Message
      {"6969696969696969696969696969696969696969696969696969696969696969",
       "0xbE93f9BacBcFFC8ee6663f2647917ed7A20a57BB"},
      // Used for Sign Transaction
      {"8140CEA58E3BEBD6174DBC589A7F70E049556233D32E44969D62E51DD0D1189A",
       "0x2166fB4e11D44100112B1124ac593081519cA1ec"}};
  std::vector<uint8_t> seed;
  EXPECT_TRUE(base::HexStringToBytes(
      "13ca6c28d26812f82db27908de0b0b7b18940cc4e9d96ebd7de190f706741489907ef65b"
      "8f9e36c31dc46e81472b6a5e40a4487e725ace445b8203f243fb8958",
      &seed));
  EthereumKeyring keyring(seed);
  size_t private_keys_size = sizeof(private_keys) / sizeof(private_keys[0]);
  for (size_t i = 0; i < private_keys_size; ++i) {
    std::vector<uint8_t> private_key;
    EXPECT_TRUE(base::HexStringToBytes(private_keys[i].key, &private_key));
    EXPECT_EQ(keyring.ImportAccount(private_key), private_keys[i].address);
  }
  EXPECT_EQ(keyring.GetImportedAccountsForTesting().size(), private_keys_size);
  // Trying to add a duplicate account
  std::vector<uint8_t> private_key0;
  EXPECT_TRUE(base::HexStringToBytes(private_keys[0].key, &private_key0));
  EXPECT_TRUE(keyring.ImportAccount(private_key0).empty());

  // SignMessage
  std::vector<uint8_t> message;
  EXPECT_TRUE(base::HexStringToBytes("68656c6c6f20776f726c64", &message));
  const std::vector<uint8_t> sig = keyring.SignMessage(
      "0xbE93f9BacBcFFC8ee6663f2647917ed7A20a57BB", message, 0, false);
  EXPECT_EQ(base::ToLowerASCII(base::HexEncode(sig)),
            "ce909e8ea6851bc36c007a0072d0524b07a3ff8d4e623aca4c71ca8e57250c4d0a"
            "3fc38fa8fbaaa81ead4b9f6bd03356b6f8bf18bccad167d78891636e1d69561b");
  EXPECT_TRUE(keyring.RemoveImportedAccount(
      "0xbE93f9BacBcFFC8ee6663f2647917ed7A20a57BB"));
  EXPECT_EQ(keyring.GetImportedAccountsForTesting().size(),
            private_keys_size - 1);
  // Delete a non existing account
  EXPECT_FALSE(keyring.RemoveImportedAccount(
      "0xbE93f9BacBcFFC8ee6663f2647917ed7A20a57BB"));
  EXPECT_FALSE(keyring.RemoveImportedAccount(""));
  EXPECT_FALSE(keyring.RemoveImportedAccount("*****0x*****"));
  EXPECT_TRUE(keyring
                  .SignMessage("0xbE93f9BacBcFFC8ee6663f2647917ed7A20a57BB",
                               message, 0, false)
                  .empty());

  // Sign Transaction
  EthTransaction tx = *EthTransaction::FromTxData(mojom::TxData::New(
      "0x09", "0x4a817c800", "0x5208",
      "0x3535353535353535353535353535353535353535", "0x0de0b6b3a7640000",
      std::vector<uint8_t>(), false, std::nullopt));
  keyring.SignTransaction("0xbE93f9BacBcFFC8ee6663f2647917ed7A20a57BB", &tx, 0);
  EXPECT_FALSE(tx.IsSigned());

  keyring.SignTransaction("0x2166fB4e11D44100112B1124ac593081519cA1ec", &tx, 0);
  EXPECT_TRUE(tx.IsSigned());

  // Try adding derived account.
  auto address = keyring.AddNewHDAccount()->address;
  EXPECT_EQ(address, "0x2166fB4e11D44100112B1124ac593081519cA1ec");
  const std::string account_0_pri_key =
      "8140cea58e3bebd6174dbc589a7f70e049556233d32e44969d62e51dd0d1189a";
  std::vector<uint8_t> private_key;
  EXPECT_TRUE(base::HexStringToBytes(account_0_pri_key, &private_key));
  EXPECT_TRUE(keyring.ImportAccount(private_key).empty());
}

TEST(EthereumKeyringUnitTest, GetPublicKeyFromX25519_XSalsa20_Poly1305) {
  EthereumKeyring keyring(*MnemonicToSeed(kMnemonic));
  auto address = keyring.AddNewHDAccount()->address;
  std::string public_encryption_key;
  EXPECT_TRUE(keyring.GetPublicKeyFromX25519_XSalsa20_Poly1305(
      address, &public_encryption_key));
  EXPECT_EQ(public_encryption_key,
            "eui9/fqCHT7aSUkKK9eooQFnOCD9COK9Mi1ZtOxIj2A=");

  // Incorrect address
  EXPECT_FALSE(keyring.GetPublicKeyFromX25519_XSalsa20_Poly1305(
      "0xbE93f9BacBcFFC8ee6663f2647917ed7A20a57BB", &public_encryption_key));
  // Invalid address
  EXPECT_FALSE(keyring.GetPublicKeyFromX25519_XSalsa20_Poly1305(
      "", &public_encryption_key));
}

TEST(EthereumKeyringUnitTest, DecryptCipherFromX25519_XSalsa20_Poly1305) {
  EthereumKeyring keyring(*MnemonicToSeed(kMnemonic));

  // pub encryption key:
  // "eui9/fqCHT7aSUkKK9eooQFnOCD9COK9Mi1ZtOxIj2A="
  auto address = keyring.AddNewHDAccount()->address;

  // {
  //   version: 'x25519-xsalsa20-poly1305',
  //   nonce: '2forT3nCPBRye9DeM1QSuWN8WvGZVpXN',
  //   ephemPublicKey: 'mjgeR52JCGDl9336uQwN29qkbYR0WLKCginBPDFtyXY=',
  //   ciphertext: 'e3xpKp2vGmpd9ecOjRJ6xltVr0Feo+Pm/C/gTMCyDg=='
  // }
  std::string ciphertext_str;
  EXPECT_TRUE(base::Base64Decode("e3xpKp2vGmpd9ecOjRJ6xltVr0Feo+Pm/C/gTMCyDg==",
                                 &ciphertext_str));
  std::vector<uint8_t> ciphertext(ciphertext_str.begin(), ciphertext_str.end());
  std::string nonce_str;
  EXPECT_TRUE(
      base::Base64Decode("2forT3nCPBRye9DeM1QSuWN8WvGZVpXN", &nonce_str));
  std::vector<uint8_t> nonce(nonce_str.begin(), nonce_str.end());

  std::string ephemeral_public_key_str;
  EXPECT_TRUE(base::Base64Decode("mjgeR52JCGDl9336uQwN29qkbYR0WLKCginBPDFtyXY=",
                                 &ephemeral_public_key_str));
  std::vector<uint8_t> ephemeral_public_key(ephemeral_public_key_str.begin(),
                                            ephemeral_public_key_str.end());

  std::optional<std::vector<uint8_t>> message_bytes;
  message_bytes = keyring.DecryptCipherFromX25519_XSalsa20_Poly1305(
      "x25519-xsalsa20-poly1305", nonce, ephemeral_public_key, ciphertext,
      address);
  EXPECT_TRUE(message_bytes.has_value());
  std::string message_str(message_bytes->begin(), message_bytes->end());
  EXPECT_EQ(message_str, "Ode to Anthony!");

  // 0 byte message
  // {
  //   version: 'x25519-xsalsa20-poly1305',
  //   nonce: 'lz+CB6jeQsUkRB+j8u9jZ37z3hun62kE',
  //   ephemPublicKey: 'InEw0fowJVqqcwtsaHjxTsAtp3kyFtyTMHgd7UrQ1Tg=',
  //   ciphertext: 'J+hZ4WZ0Z+YmkOsz2unA+w=='
  // }
  EXPECT_TRUE(base::Base64Decode("J+hZ4WZ0Z+YmkOsz2unA+w==", &ciphertext_str));
  ciphertext =
      std::vector<uint8_t>(ciphertext_str.begin(), ciphertext_str.end());
  EXPECT_TRUE(base::Base64Decode("InEw0fowJVqqcwtsaHjxTsAtp3kyFtyTMHgd7UrQ1Tg=",
                                 &ephemeral_public_key_str));
  ephemeral_public_key = std::vector<uint8_t>(ephemeral_public_key_str.begin(),
                                              ephemeral_public_key_str.end());
  EXPECT_TRUE(
      base::Base64Decode("lz+CB6jeQsUkRB+j8u9jZ37z3hun62kE", &nonce_str));
  nonce = std::vector<uint8_t>(nonce_str.begin(), nonce_str.end());
  message_bytes = keyring.DecryptCipherFromX25519_XSalsa20_Poly1305(
      "x25519-xsalsa20-poly1305", nonce, ephemeral_public_key, ciphertext,
      address);
  message_str = std::string(message_bytes->begin(), message_bytes->end());
  EXPECT_TRUE(message_str.empty());

  // Wrong version
  EXPECT_EQ(keyring.DecryptCipherFromX25519_XSalsa20_Poly1305(
                "x25519-xsalsa20-poly1306", nonce, ephemeral_public_key,
                ciphertext, address),
            std::nullopt);

  // Empty nonce
  EXPECT_EQ(keyring.DecryptCipherFromX25519_XSalsa20_Poly1305(
                "x25519-xsalsa20-poly1305", std::vector<uint8_t>(),
                ephemeral_public_key, ciphertext, address),
            std::nullopt);

  // Empty public key
  EXPECT_EQ(keyring.DecryptCipherFromX25519_XSalsa20_Poly1305(
                "x25519-xsalsa20-poly1305", nonce, std::vector<uint8_t>(),
                ciphertext, address),
            std::nullopt);

  // Empty ciphertext
  EXPECT_EQ(keyring.DecryptCipherFromX25519_XSalsa20_Poly1305(
                "x25519-xsalsa20-poly1305", nonce, ephemeral_public_key,
                std::vector<uint8_t>(), address),
            std::nullopt);

  // Wrong nonce
  std::vector<uint8_t> bad_nonce = nonce;
  bad_nonce[0] = 0;
  EXPECT_EQ(keyring.DecryptCipherFromX25519_XSalsa20_Poly1305(
                "x25519-xsalsa20-poly1305", bad_nonce, ephemeral_public_key,
                ciphertext, address),
            std::nullopt);

  // Wrong public key
  std::vector<uint8_t> bad_ephemeral_public_key = ephemeral_public_key;
  bad_ephemeral_public_key[0] = 0;
  EXPECT_EQ(keyring.DecryptCipherFromX25519_XSalsa20_Poly1305(
                "x25519-xsalsa20-poly1305", nonce, bad_ephemeral_public_key,
                ciphertext, address),
            std::nullopt);

  // Wrong ciphertext
  std::vector<uint8_t> bad_ciphertext = ciphertext;
  bad_ciphertext[0] = 0;
  EXPECT_EQ(keyring.DecryptCipherFromX25519_XSalsa20_Poly1305(
                "x25519-xsalsa20-poly1305", nonce, ephemeral_public_key,
                bad_ciphertext, address),
            std::nullopt);
}

}  // namespace brave_wallet
