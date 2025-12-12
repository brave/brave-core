/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_KEYRING_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_KEYRING_H_

#include "base/containers/flat_map.h"
#include "brave/components/brave_wallet/browser/internal/hd_key_sr25519.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace brave_wallet {

inline constexpr size_t kPolkadotSeedSize = 32;

class PolkadotKeyring {
 public:
  // Construct the keyring for Polkadot using the provided seed, derived from
  // the bip39::MnemonicToEntropyToSeed() method.
  PolkadotKeyring(base::span<const uint8_t, kPolkadotSeedSize> seed,
                  mojom::KeyringId keyring_id);
  ~PolkadotKeyring();

  // Get the address of the account denoted by `//<network>//<account_index>`,
  // which is the SS58-encoded public key for this particular derivation. Many
  // parachains use their own ss58 prefix, which the caller can supply.
  // Unified addressing uses 0 as the default prefix.
  std::string GetAddress(uint32_t account_index, uint16_t prefix);

  // Get the public key associated with the account denoted by
  // `//<network>//<account_index>`.
  std::array<uint8_t, kSr25519PublicKeySize> GetPublicKey(
      uint32_t account_index);

  // Use the derived account `account_index` to sign the provided message.
  std::array<uint8_t, kSr25519SignatureSize> SignMessage(
      base::span<const uint8_t> message,
      uint32_t account_index);

  // Verify that the provided signature is associated with the given message,
  // for the account denoted by `account_index`.
  [[nodiscard]] bool VerifyMessage(
      base::span<const uint8_t, kSr25519SignatureSize> signature,
      base::span<const uint8_t> message,
      uint32_t account_index);

  // Helper that tells us if this keyring is intended for the `//polkadot`
  // mainnet or the `//westend` testnet.
  bool IsTestnet() const;

  mojom::KeyringId keyring_id() const { return keyring_id_; }

  std::optional<std::string> AddNewHDAccount(uint32_t index);

  // Encodes the private key for export in JSON format.
  // Returns a JSON string with encoded key, encoding metadata, and address.
  // The seed is encrypted using xsalsa20-poly1305 with a password-derived key.
  std::optional<std::string> EncodePrivateKeyForExport(
      uint32_t account_index,
      const std::string& password);

  // Sets random bytes for testing for private key export.
  void SetRandBytesForTesting(const std::vector<uint8_t>& seed_bytes,
                              const std::vector<uint8_t>& nonce_bytes);

 private:
  HDKeySr25519& EnsureKeyPair(uint32_t account_index);

  HDKeySr25519 root_account_key_;
  mojom::KeyringId keyring_id_;
  base::flat_map<uint32_t, HDKeySr25519> secondary_keys_;

  std::optional<std::vector<uint8_t>> rand_seed_bytes_for_testing_;
  std::optional<std::vector<uint8_t>> rand_nonce_bytes_for_testing_;
};
}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_KEYRING_H_
