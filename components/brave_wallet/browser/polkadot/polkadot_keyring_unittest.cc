/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_keyring.h"

#include <string_view>

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/test/bind.h"
#include "brave/components/brave_wallet/browser/bip39.h"
#include "brave/components/brave_wallet/browser/blockchain_registry.h"
#include "brave/components/brave_wallet/common/encoding_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {
namespace {

// All derived from the polkadot-sdk using:
// let pair =
//   polkadot_sdk::sp_core::sr25519::Pair::from_string(
//     DEV_PHRASE, None).unwrap();

constexpr const char kDevPhrase[] =
    "bottom drive obey lake curtain smoke basket hold race lonely fit walk";

constexpr char const kDevSeed[] =
    "FAC7959DBFE72F052E5A0C3C8D6530F202B02FD8F9F5CA3580EC8DEB7797479E0B9F67282F"
    "42D1214E457243B9EB38A0A5ED13DE66C01CFB213A6FE73CEDEF5A";

constexpr char const* kDevPubKey =
    "46EBDDEF8CD9BB167DC30878D7113B7E168E6F0646BEFFD77D69D39BAD76B47A";

// From:
// let s = pair.public().to_string();
// assert_eq!(s, "5DfhGyQdFobKM8NsWvEeAKk5EQQgYe9AydgJ7rMB6E1EqRzV");
//
// See also:
// https://github.com/paritytech/polkadot-sdk/blob/0d765ce37b258640a6eeb575f6bff76d6a7b7c46/substrate/primitives/core/src/crypto.rs#L49
constexpr const char kDevAddress[] =
    "5DfhGyQdFobKM8NsWvEeAKk5EQQgYe9AydgJ7rMB6E1EqRzV";

std::array<uint8_t, kSr25519SignatureSize> ToSignature(std::string_view hex) {
  std::array<uint8_t, kSr25519SignatureSize> signature_bytes;
  base::HexStringToSpan(hex, signature_bytes);
  return signature_bytes;
}

bool IsAddressAllowed(const std::string&) {
  return true;
}

}  // namespace

TEST(PolkadotKeyring, GenerateRoot) {
  auto seed = bip39::MnemonicToEntropyToSeed(kDevPhrase).value();
  EXPECT_EQ(base::HexEncode(seed), kDevSeed);

  // Verify that our seed can be used to derive a keypair with a public key that
  // matches the test vector.
  auto keypair = HDKeySr25519::GenerateFromSeed(
      base::span(seed).first<kSr25519SeedSize>());
  auto pubkey = keypair.GetPublicKey();

  // https://wiki.polkadot.com/learn/learn-account-advanced/#for-the-curious-how-prefixes-work
  const uint16_t substrate_prefix = 42;

  Ss58Address addr;
  addr.prefix = substrate_prefix;
  base::span(addr.public_key)
      .copy_from_nonoverlapping(base::span<uint8_t const>(pubkey));

  EXPECT_EQ(base::HexEncode(pubkey), kDevPubKey);

  auto dev_addr = addr.Encode().value();
  EXPECT_EQ(dev_addr, kDevAddress);
}

TEST(PolkadotKeyring, Constructor) {
  auto seed = bip39::MnemonicToEntropyToSeed(kDevPhrase).value();

  PolkadotKeyring keyring(base::span(seed).first<kPolkadotSeedSize>(),
                          mojom::KeyringId::kPolkadotMainnet,
                          base::BindRepeating(IsAddressAllowed));
  EXPECT_FALSE(keyring.IsTestnet());

  PolkadotKeyring keyring2(base::span(seed).first<kPolkadotSeedSize>(),
                           mojom::KeyringId::kPolkadotTestnet,
                           base::BindRepeating(IsAddressAllowed));
  EXPECT_TRUE(keyring2.IsTestnet());
}

TEST(PolkadotKeyring, GetAddress) {
  // Derived from the polkadot-sdk using:
  // clang-format off
  //
  // polkadot_sdk::sp_core::crypto::set_default_ss58_version(
  //     Ss58AddressFormat::from(
  //         sp_core::crypto::Ss58AddressFormatRegistry::PolkadotAccount
  // ));
  //
  // let pair = polkadot_sdk::sp_core::sr25519::Pair::from_string(
  //     &format!("{DEV_PHRASE}"),
  //     None,
  // )
  // .unwrap();
  //
  // let s = pair.public().to_string();
  // assert_eq!(s, "12bzRJfh7arnnfPPUZHeJUaE62QLEwhK48QnH9LXeK2m1iZU");
  //
  // clang-format on

  // Mainnet.
  {
    auto seed = bip39::MnemonicToEntropyToSeed(kDevPhrase).value();
    PolkadotKeyring keyring(base::span(seed).first<kPolkadotSeedSize>(),
                            mojom::KeyringId::kPolkadotMainnet,
                            base::BindRepeating(IsAddressAllowed));
    ASSERT_TRUE(keyring.AddNewHDAccount(0));
    ASSERT_TRUE(keyring.AddNewHDAccount(1));

    EXPECT_EQ(keyring.GetAddress(0, 0u),
              "12bzRJfh7arnnfPPUZHeJUaE62QLEwhK48QnH9LXeK2m1iZU");

    // DEV_PHRASE//0
    EXPECT_EQ(keyring.GetAddress(1, 0u),
              "1yMmfLti1k3huRQM2c47WugwonQMqTvQ2GUFxnU7Pcs7xPo");
    // This test exercises the caching codepath in the GetAddress
    // implementation.
    EXPECT_EQ(keyring.GetAddress(0, 0u),
              "12bzRJfh7arnnfPPUZHeJUaE62QLEwhK48QnH9LXeK2m1iZU");
  }

  // Testnet.
  // Generated using subkey, provided by:
  // https://github.com/paritytech/polkadot-sdk/tree/823a550f4be744bd1ba9e47a2ce82ede6e8aa4bd
  //
  // clang-format off
  // cargo run -p subkey --bin subkey -- inspect --scheme sr25519 --network substrate "bottom drive obey lake curtain smoke basket hold race lonely fit walk//*"
  // clang-format on
  {
    auto seed = bip39::MnemonicToEntropyToSeed(kDevPhrase).value();
    PolkadotKeyring keyring(base::span(seed).first<kPolkadotSeedSize>(),
                            mojom::KeyringId::kPolkadotTestnet,
                            base::BindRepeating(IsAddressAllowed));
    ASSERT_TRUE(keyring.AddNewHDAccount(0));
    ASSERT_TRUE(keyring.AddNewHDAccount(1));

    EXPECT_EQ(keyring.GetAddress(0, 42u),
              "5DfhGyQdFobKM8NsWvEeAKk5EQQgYe9AydgJ7rMB6E1EqRzV");
    EXPECT_EQ(keyring.GetAddress(1, 42u),
              "5D34dL5prEUaGNQtPPZ3yN5Y6BnkfXunKXXz6fo7ZJbLwRRH");
    // This test exercises the caching codepath in the GetAddress
    // implementation.
    EXPECT_EQ(keyring.GetAddress(0, 42u),
              "5DfhGyQdFobKM8NsWvEeAKk5EQQgYe9AydgJ7rMB6E1EqRzV");
  }
}

TEST(PolkadotKeyring, AddHDAccount) {
  // Derived from the polkadot-sdk using:
  // clang-format off
  //
  // polkadot_sdk::sp_core::crypto::set_default_ss58_version(
  //     Ss58AddressFormat::from(
  //         sp_core::crypto::Ss58AddressFormatRegistry::PolkadotAccount
  // ));
  //
  // let pair = polkadot_sdk::sp_core::sr25519::Pair::from_string(
  //     &format!("{DEV_PHRASE}"),
  //     None,
  // )
  // .unwrap();
  //
  // let s = pair.public().to_string();
  // assert_eq!(s, "12bzRJfh7arnnfPPUZHeJUaE62QLEwhK48QnH9LXeK2m1iZU");
  //
  // clang-format on

  // Mainnet.
  {
    auto seed = bip39::MnemonicToEntropyToSeed(kDevPhrase).value();
    PolkadotKeyring keyring(base::span(seed).first<kPolkadotSeedSize>(),
                            mojom::KeyringId::kPolkadotMainnet,
                            base::BindRepeating(IsAddressAllowed));

    EXPECT_EQ(keyring.AddNewHDAccount(0u).value(),
              "12bzRJfh7arnnfPPUZHeJUaE62QLEwhK48QnH9LXeK2m1iZU");
    // "DEV_PHRASE//0"
    EXPECT_EQ(keyring.AddNewHDAccount(1u).value(),
              "1yMmfLti1k3huRQM2c47WugwonQMqTvQ2GUFxnU7Pcs7xPo");
  }

  // Testnet.
  // Generated using subkey:
  // clang-format off
  // cargo run -p subkey --bin subkey -- inspect --scheme sr25519 --network substrate "bottom drive obey lake curtain smoke basket hold race lonely fit walk"
  // clang-format on
  {
    auto seed = bip39::MnemonicToEntropyToSeed(kDevPhrase).value();
    PolkadotKeyring keyring(base::span(seed).first<kPolkadotSeedSize>(),
                            mojom::KeyringId::kPolkadotTestnet,
                            base::BindRepeating(IsAddressAllowed));

    EXPECT_EQ(keyring.AddNewHDAccount(0u).value(),
              "5DfhGyQdFobKM8NsWvEeAKk5EQQgYe9AydgJ7rMB6E1EqRzV");
    EXPECT_EQ(keyring.AddNewHDAccount(1u).value(),
              "5D34dL5prEUaGNQtPPZ3yN5Y6BnkfXunKXXz6fo7ZJbLwRRH");
  }
}

TEST(PolkadotKeyring, GetPublicKey) {
  // Derived from the polkadot-sdk using:
  // clang-format off
  //
  // let pair = polkadot_sdk::sp_core::sr25519::Pair::from_string(
  //     &format!("{DEV_PHRASE}"),
  //     None,
  // )
  // .unwrap();
  // let s = bytes_to_hex(pair.public().as_bytes_ref());
  // assert_eq!(
  //     s,
  //     "9C9C968EBB31417A36BEC0908ECD9EB6E847B44821E521DDA9ADD8C418EF7C30"
  // );
  //
  // clang-format on

  // Mainnet.
  {
    auto seed = bip39::MnemonicToEntropyToSeed(kDevPhrase).value();
    PolkadotKeyring keyring(base::span(seed).first<kPolkadotSeedSize>(),
                            mojom::KeyringId::kPolkadotMainnet,
                            base::BindRepeating(IsAddressAllowed));
    ASSERT_TRUE(keyring.AddNewHDAccount(0));

    auto pubkey = keyring.GetPublicKey(0);
    ASSERT_TRUE(pubkey.has_value());

    constexpr char const* kPublicKey =
        "46ebddef8cd9bb167dc30878d7113b7e168e6f0646beffd77d69d39bad76b47a";
    EXPECT_EQ(base::HexEncodeLower(*pubkey), kPublicKey);
  }

  // Testnet.
  // Generated using subkey:
  // clang-format off
  // cargo run -p subkey --bin subkey -- inspect --scheme sr25519 --network substrate "bottom drive obey lake curtain smoke basket hold race lonely fit walk"
  // clang-format on
  {
    auto seed = bip39::MnemonicToEntropyToSeed(kDevPhrase).value();
    PolkadotKeyring keyring(base::span(seed).first<kPolkadotSeedSize>(),
                            mojom::KeyringId::kPolkadotTestnet,
                            base::BindRepeating(IsAddressAllowed));
    ASSERT_TRUE(keyring.AddNewHDAccount(0));

    auto pubkey = keyring.GetPublicKey(0);
    ASSERT_TRUE(pubkey.has_value());

    constexpr char const* kPublicKey =
        "46ebddef8cd9bb167dc30878d7113b7e168e6f0646beffd77d69d39bad76b47a";
    EXPECT_EQ(base::HexEncodeLower(*pubkey), kPublicKey);
  }
}

TEST(PolkadotKeyring, SignAndVerifyMessage) {
  auto message = base::byte_span_from_cstring("hello, world!");

  // Mainnet.
  {
    auto seed = bip39::MnemonicToEntropyToSeed(kDevPhrase).value();
    PolkadotKeyring keyring(base::span(seed).first<kPolkadotSeedSize>(),
                            mojom::KeyringId::kPolkadotMainnet,
                            base::BindRepeating(IsAddressAllowed));
    ASSERT_TRUE(keyring.AddNewHDAccount(0));
    ASSERT_TRUE(keyring.AddNewHDAccount(1));

    auto signature = keyring.SignMessage(message, 0);
    ASSERT_TRUE(signature);

    auto verified = keyring.VerifyMessage(*signature, message, 0);
    EXPECT_TRUE(verified);

    verified = keyring.VerifyMessage(*signature, message, 1);
    EXPECT_FALSE(verified);

    verified = keyring.VerifyMessage(*signature, message, 1234);
    EXPECT_FALSE(verified);
  }

  // Testnet.
  {
    auto seed = bip39::MnemonicToEntropyToSeed(kDevPhrase).value();
    PolkadotKeyring keyring(base::span(seed).first<kPolkadotSeedSize>(),
                            mojom::KeyringId::kPolkadotTestnet,
                            base::BindRepeating(IsAddressAllowed));
    ASSERT_TRUE(keyring.AddNewHDAccount(0));
    ASSERT_TRUE(keyring.AddNewHDAccount(1));

    auto signature = keyring.SignMessage(message, 0);
    ASSERT_TRUE(signature);

    auto verified = keyring.VerifyMessage(*signature, message, 0);
    EXPECT_TRUE(verified);

    verified = keyring.VerifyMessage(*signature, message, 1);
    EXPECT_FALSE(verified);

    verified = keyring.VerifyMessage(*signature, message, 1234);
    EXPECT_FALSE(verified);
  }
}

TEST(PolkadotKeyring, VerifyMessage) {
  auto message = base::byte_span_from_cstring("hello, world!");

  // Testnet.
  // Generated using subkey:
  // clang-format off
  // cargo run -p subkey --bin subkey -- sign --suri "bottom drive obey lake curtain smoke basket hold race lonely fit walk" --message "hello, world!"
  // clang-format on
  {
    auto seed = bip39::MnemonicToEntropyToSeed(kDevPhrase).value();
    PolkadotKeyring keyring(base::span(seed).first<kPolkadotSeedSize>(),
                            mojom::KeyringId::kPolkadotTestnet,
                            base::BindRepeating(IsAddressAllowed));
    ASSERT_TRUE(keyring.AddNewHDAccount(0));
    ASSERT_TRUE(keyring.AddNewHDAccount(1));

    std::string signature_hex =
        "a490725b1fc73fea10414c29d9fedc42855cbdbb8fac56bae3c671d17218a6793c82cd"
        "fccb51b435f4a5f8fbcacaa048691c03368e9886f2f5475f7f4f68808c";

    bool verified =
        keyring.VerifyMessage(ToSignature(signature_hex), message, 0);
    EXPECT_TRUE(verified);

    verified = keyring.VerifyMessage(ToSignature(signature_hex), message, 1);
    EXPECT_FALSE(verified);

    std::string signature_hex2 =
        "32890817c929ed60f093174a2c08e408bb9e6b0b87ec4df40a5cf8a8f4556118de5e7d"
        "824e6c4fe5b73008b63f6ac85a4164f76698f0b90d1a65eaa9be861f8f";

    verified = keyring.VerifyMessage(ToSignature(signature_hex2), message, 0);
    EXPECT_TRUE(verified);

    // Test with wrong account index - should fail
    verified = keyring.VerifyMessage(ToSignature(signature_hex2), message, 1);
    EXPECT_FALSE(verified);

    // Test with wrong signature data - should fail
    std::string wrong_signature_hex =
        "fffffff512301fb068ab1de33c00dc2a9b020f0f446b5c1ea89d3f4a04a5b05c8206c5"
        "5dccd8419019fc86f4b8d177cdef035fc36a0be8755423ba7377927d8d";

    verified =
        keyring.VerifyMessage(ToSignature(wrong_signature_hex), message, 0);
    EXPECT_FALSE(verified);
  }

  // Mainnet.
  // Generated using subkey:
  // clang-format off
  // cargo run -p subkey --bin subkey -- sign --suri "bottom drive obey lake curtain smoke basket hold race lonely fit walk" --message "hello, world!"
  // clang-format on
  {
    auto seed = bip39::MnemonicToEntropyToSeed(kDevPhrase).value();
    PolkadotKeyring keyring(base::span(seed).first<kPolkadotSeedSize>(),
                            mojom::KeyringId::kPolkadotMainnet,
                            base::BindRepeating(IsAddressAllowed));
    ASSERT_TRUE(keyring.AddNewHDAccount(0));
    ASSERT_TRUE(keyring.AddNewHDAccount(1));

    // Test with first mainnet signature vector
    std::string signature_hex =
        "b21560c19d6771af374ddb15bd5dcd9cabe723c44063aae8157fca46d2bba16ae1ca9f"
        "40b15ce66d9c1f7a0e961a8bf2452200534bde7a03d7531b2b7fa11186";

    bool verified =
        keyring.VerifyMessage(ToSignature(signature_hex), message, 0);
    EXPECT_TRUE(verified);

    // Test with wrong account index - should fail
    verified = keyring.VerifyMessage(ToSignature(signature_hex), message, 1);
    EXPECT_FALSE(verified);

    // Test with second mainnet signature vector
    std::string signature_hex2 =
        "4e96e323b240c68d361f3a3439156dbad35d9cbae68f735c7c06b7803933f217b79b73"
        "eade9507fd901cf28b903b0a420fede4312594ac833ef92825e44b8480";

    verified = keyring.VerifyMessage(ToSignature(signature_hex2), message, 0);
    EXPECT_TRUE(verified);

    // Test with wrong account index - should fail
    verified = keyring.VerifyMessage(ToSignature(signature_hex2), message, 1);
    EXPECT_FALSE(verified);

    // Test with wrong signature data - should fail
    std::string wrong_signature_hex =
        "ffffff9387d0d89f0e22df4d90f5f8ccaa9e6b974df6cad39c79764c655ea924378536"
        "559e2d925ee534f31583511bf5050d9cb881ad51ce4607a5ad3c75e78a";

    verified =
        keyring.VerifyMessage(ToSignature(wrong_signature_hex), message, 0);
    EXPECT_FALSE(verified);
  }
}

TEST(PolkadotKeyring, AddNewHDAccount_RestrictedAddress) {
  auto* registry = BlockchainRegistry::GetInstance();
  CHECK(registry);

  auto seed = bip39::MnemonicToEntropyToSeed(kDevPhrase).value();
  PolkadotKeyring keyring(
      base::span(seed).first<kPolkadotSeedSize>(),
      mojom::KeyringId::kPolkadotMainnet,
      base::BindLambdaForTesting([=](const std::string& address) {
        return !registry->IsRestrictedAddress(address);
      }));

  // Add an account to get its address.
  auto address = keyring.AddNewHDAccount(0);
  ASSERT_TRUE(address);
  const std::string address_to_restrict = *address;

  PolkadotKeyring keyring2(
      base::span(seed).first<kPolkadotSeedSize>(),
      mojom::KeyringId::kPolkadotMainnet,
      base::BindLambdaForTesting([=](const std::string& address) {
        return !registry->IsRestrictedAddress(address);
      }));
  {
    // Add address to restricted list.
    BlockchainRegistry::ScopedRestrictedAddressesForTesting scoped_restricted(
        {base::ToLowerASCII(address_to_restrict)});

    // Try to add account again - should fail because it generates the same
    // address Note: PolkadotKeyring doesn't have RemoveHDAccount, so we test by
    // trying to add at index 1, which should succeed, then try index 0 again.
    auto result1 = keyring.AddNewHDAccount(1);
    EXPECT_TRUE(result1) << "Non-restricted address should succeed";

    // Now try to add at index 0 again - this should fail because the address
    // at index 0 is already in the restricted list
    // Actually, we can't test this directly because AddNewHDAccount doesn't
    // allow adding at an index that's already been used. Instead, let's test
    // by creating a new keyring and trying to add at index 0.
    auto result2 = keyring2.AddNewHDAccount(0);
    EXPECT_FALSE(result2) << "Restricted Polkadot address should be rejected";
  }

  // Prove that we didn't leave any remnant phantom keypairs.
  auto result2 = keyring2.AddNewHDAccount(0);
  EXPECT_TRUE(result2);
}

TEST(PolkadotKeyring, TalismanParity) {
  // These can be verified by extending the tests here:
  // https://github.com/TalismanSociety/talisman/blob/8e1f3eb6815d9628abf764cc33f09e632ba6262d/packages/crypto/src/derivation/derive.test.ts
  //
  // to include:
  //
  // const checkPolkadotDerivedAddress = async (
  //   mnemonic: string,
  //   derivationPath: string,
  //   curve: KeypairCurve,
  //   address: string
  // ) => {
  //   const entropy = mnemonicToEntropy(mnemonic)
  //   const seed = await entropyToSeed(entropy, curve)
  //   const secret = deriveKeypair(seed, derivationPath, curve)
  //   const format = addressEncodingFromCurve(curve)
  //   expect(address).toEqual(
  //    addressFromPublicKey(
  //      secret.publicKey, format, {ss58Prefix: 0}))
  // }
  //
  // Verify by running the tests using:
  // pnpm test packages/crypto/src/derivation/derive.test.ts

  static constexpr std::string_view kSeedPhrase =
      "educate orient stock gate struggle glass awkward sad crop tornado "
      "august during";

  auto seed = bip39::MnemonicToEntropyToSeed(kSeedPhrase).value();
  PolkadotKeyring keyring(base::span(seed).first<kPolkadotSeedSize>(),
                          mojom::KeyringId::kPolkadotMainnet,
                          base::BindRepeating(IsAddressAllowed));
  ASSERT_TRUE(keyring.AddNewHDAccount(0));
  ASSERT_TRUE(keyring.AddNewHDAccount(1));
  ASSERT_TRUE(keyring.AddNewHDAccount(2));
  ASSERT_TRUE(keyring.AddNewHDAccount(3));
  ASSERT_TRUE(keyring.AddNewHDAccount(4));

  // <mnemonic>
  auto address = keyring.GetAddress(0, 0);
  ASSERT_TRUE(address.has_value());
  EXPECT_EQ(*address, "16Pp1xFjkz1ruD63F84Nk6h2X4ijWfAhtrs66vEAgLWUkXp7");

  // <mnemonic>//0
  address = keyring.GetAddress(1, 0);
  ASSERT_TRUE(address.has_value());
  EXPECT_EQ(*address, "1RqoB2myWtys99XJYBhRQFoZx6N4PghsphpMCWQ4xkAxmH8");

  // <mnemonic>//1
  address = keyring.GetAddress(2, 0);
  ASSERT_TRUE(address.has_value());
  EXPECT_EQ(*address, "15DgT3UXTHyC5BXWiVo8zT37X3hA2mjrN3jzncrsjAn9WgKr");

  // <mnemonic>//2
  address = keyring.GetAddress(3, 0);
  ASSERT_TRUE(address.has_value());
  EXPECT_EQ(*address, "15D7hSjTdnHBwbavvmFwQSN8FGdz6b6LhH5QvAaeoDmvCrx5");
}

}  // namespace brave_wallet
