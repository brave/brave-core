/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_import_keyring.h"

#include <array>
#include <string_view>

#include "brave/components/brave_wallet/browser/bip39.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_keyring.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_constants.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace {

// Same as polkadot_keyring_unittest: derived from polkadot-sdk / subkey.
constexpr char kDevPhrase[] =
    "bottom drive obey lake curtain smoke basket hold race lonely fit walk";

// Expected SS58 addresses for kDevPhrase, matching PolkadotKeyring test
// vectors. Mainnet (prefix 0).
constexpr const char kMainnetAddress0[] =
    "14YLzDFZTwnkcJkFij4Km7g5LdkLqKHy47xYGPN6HsLJpfnb";
constexpr const char kMainnetAddress1[] =
    "14QspRnocdHvoDfFnGvvYwskrsSWG8nneJ2BAQ2oSMcsQUxB";
// Testnet (prefix 42).
constexpr const char kTestnetAddress0[] =
    "5HGiBcFgEBMgT6GEuo9SA98sBnGgwHtPKDXiUukT6aqCrKEx";
constexpr const char kTestnetAddress1[] =
    "5CofVLAGjwvdGXvBiP6ddtZYMVbhT5Xke8ZrshUpj2ZXAnND";

}  // namespace

TEST(PolkadotImportKeyringTest, AddAccountAndGetAddress) {
  auto seed = bip39::MnemonicToEntropyToSeed(kDevPhrase).value();

  // Mainnet: import keyring must produce same addresses as HD keyring.
  {
    PolkadotKeyring hd_keyring(base::span(seed).first<kPolkadotSeedSize>(),
                               mojom::KeyringId::kPolkadotMainnet);
    PolkadotImportKeyring import_keyring(mojom::KeyringId::kPolkadotImport);

    EXPECT_TRUE(
        import_keyring.AddAccount(0, hd_keyring.GetPkcs8KeyForTesting(0)));
    EXPECT_EQ(import_keyring.GetAccountAddress(0).value(), kMainnetAddress0);
    EXPECT_EQ(import_keyring.GetAddress(0, 0u).value(), kMainnetAddress0);

    EXPECT_TRUE(
        import_keyring.AddAccount(1, hd_keyring.GetPkcs8KeyForTesting(1)));
    EXPECT_EQ(import_keyring.GetAccountAddress(1).value(), kMainnetAddress1);
    EXPECT_EQ(import_keyring.GetAddress(1, 0u).value(), kMainnetAddress1);
  }

  // Testnet: import keyring must produce same addresses as HD keyring.
  {
    PolkadotKeyring hd_keyring(base::span(seed).first<kPolkadotSeedSize>(),
                               mojom::KeyringId::kPolkadotTestnet);
    PolkadotImportKeyring import_keyring(
        mojom::KeyringId::kPolkadotImportTestnet);

    EXPECT_TRUE(
        import_keyring.AddAccount(0, hd_keyring.GetPkcs8KeyForTesting(0)));
    EXPECT_EQ(import_keyring.GetAccountAddress(0).value(), kTestnetAddress0);
    EXPECT_EQ(import_keyring.GetAddress(0, 42u).value(), kTestnetAddress0);

    EXPECT_TRUE(
        import_keyring.AddAccount(1, hd_keyring.GetPkcs8KeyForTesting(1)));
    EXPECT_EQ(import_keyring.GetAccountAddress(1).value(), kTestnetAddress1);
    EXPECT_EQ(import_keyring.GetAddress(1, 42u).value(), kTestnetAddress1);
  }
}

TEST(PolkadotImportKeyringTest, AddAccountFails) {
  auto seed = bip39::MnemonicToEntropyToSeed(kDevPhrase).value();
  PolkadotKeyring hd_keyring(base::span(seed).first<kPolkadotSeedSize>(),
                             mojom::KeyringId::kPolkadotMainnet);
  auto pkcs8 = hd_keyring.GetPkcs8KeyForTesting(0);

  PolkadotImportKeyring keyring(mojom::KeyringId::kPolkadotImport);
  EXPECT_TRUE(keyring.AddAccount(0, pkcs8));
  EXPECT_FALSE(keyring.AddAccount(0, pkcs8));

  std::array<uint8_t, kSr25519Pkcs8Size> invalid_pkcs8 = {};
  EXPECT_FALSE(keyring.AddAccount(1, invalid_pkcs8));
}

TEST(PolkadotImportKeyringTest, RemoveAccount) {
  auto seed = bip39::MnemonicToEntropyToSeed(kDevPhrase).value();
  PolkadotKeyring hd_keyring(base::span(seed).first<kPolkadotSeedSize>(),
                             mojom::KeyringId::kPolkadotMainnet);
  auto pkcs8 = hd_keyring.GetPkcs8KeyForTesting(0);

  PolkadotImportKeyring keyring(mojom::KeyringId::kPolkadotImport);
  EXPECT_TRUE(keyring.AddAccount(0, pkcs8));
  EXPECT_TRUE(keyring.GetAddress(0, kSubstratePrefix).has_value());

  EXPECT_TRUE(keyring.RemoveAccount(0));
  EXPECT_FALSE(keyring.GetAddress(0, kSubstratePrefix).has_value());

  EXPECT_FALSE(keyring.RemoveAccount(0));
}

TEST(PolkadotImportKeyringTest, GetPublicKey) {
  auto seed = bip39::MnemonicToEntropyToSeed(kDevPhrase).value();
  PolkadotKeyring hd_keyring(base::span(seed).first<kPolkadotSeedSize>(),
                             mojom::KeyringId::kPolkadotMainnet);
  auto pkcs8 = hd_keyring.GetPkcs8KeyForTesting(0);

  PolkadotImportKeyring import_keyring(mojom::KeyringId::kPolkadotImport);
  EXPECT_TRUE(import_keyring.AddAccount(0, pkcs8));

  auto hd_pubkey = hd_keyring.GetPublicKey(0);
  auto import_pubkey = import_keyring.GetPublicKey(0);
  ASSERT_TRUE(import_pubkey.has_value());
  EXPECT_EQ(*import_pubkey, hd_pubkey);

  EXPECT_FALSE(import_keyring.GetPublicKey(1).has_value());
}

TEST(PolkadotImportKeyringTest, SignMessage) {
  auto seed = bip39::MnemonicToEntropyToSeed(kDevPhrase).value();
  PolkadotKeyring hd_keyring(base::span(seed).first<kPolkadotSeedSize>(),
                             mojom::KeyringId::kPolkadotMainnet);
  auto pkcs8 = hd_keyring.GetPkcs8KeyForTesting(0);

  PolkadotImportKeyring import_keyring(mojom::KeyringId::kPolkadotImport);
  EXPECT_TRUE(import_keyring.AddAccount(0, pkcs8));

  auto message = base::byte_span_from_cstring("hello, polkadot import");
  auto signature = import_keyring.SignMessage(message, 0);
  ASSERT_TRUE(signature.has_value());
  EXPECT_TRUE(hd_keyring.VerifyMessage(*signature, message, 0));

  EXPECT_FALSE(import_keyring.SignMessage(message, 1).has_value());
}

TEST(PolkadotImportKeyringTest, EncodePrivateKeyForExportRoundtrip) {
  auto seed = bip39::MnemonicToEntropyToSeed(kDevPhrase).value();
  PolkadotKeyring hd_keyring(base::span(seed).first<kPolkadotSeedSize>(),
                             mojom::KeyringId::kPolkadotMainnet);
  auto pkcs8 = hd_keyring.GetPkcs8KeyForTesting(0);

  PolkadotImportKeyring import_keyring(mojom::KeyringId::kPolkadotImport);
  EXPECT_TRUE(import_keyring.AddAccount(0, pkcs8));

  const std::string kPassword = "export_password_123";
  auto encoded = import_keyring.EncodePrivateKeyForExport(0, kPassword);
  ASSERT_TRUE(encoded.has_value());

  auto decoded = DecodePrivateKeyFromExport(*encoded, kPassword);
  ASSERT_TRUE(decoded.has_value());
  EXPECT_EQ(*decoded, pkcs8);
}

TEST(PolkadotImportKeyringTest,
     EncodePrivateKeyForExportFailsForMissingAccount) {
  PolkadotImportKeyring keyring(mojom::KeyringId::kPolkadotImport);
  EXPECT_FALSE(keyring.EncodePrivateKeyForExport(0, "password").has_value());
}

TEST(PolkadotImportKeyringTest, IsTestnet) {
  PolkadotImportKeyring mainnet_keyring(mojom::KeyringId::kPolkadotImport);
  EXPECT_FALSE(mainnet_keyring.IsTestnet());

  PolkadotImportKeyring testnet_keyring(
      mojom::KeyringId::kPolkadotImportTestnet);
  EXPECT_TRUE(testnet_keyring.IsTestnet());
}

}  // namespace brave_wallet
