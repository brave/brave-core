/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_KEYRING_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_KEYRING_H_

#include <optional>
#include <string_view>

#include "base/containers/flat_map.h"
#include "brave/components/brave_wallet/browser/internal/hd_key_sr25519.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace brave_wallet {

class PolkadotKeyring {
 public:
  // The polkadot-sdk derives seeds from mnemonics in a way divergent from
  // normal BIP-39 routines. Instead, it hashes the derived entropy instead of
  // the mnemonic so we need a special routine just for Polkadot:
  // https://github.com/paritytech/polkadot-sdk/blob/beb9030b249cc078b3955232074a8495e7e0302a/substrate/primitives/core/src/crypto.rs#L866-L883
  // https://github.com/paritytech/polkadot-sdk/blob/beb9030b249cc078b3955232074a8495e7e0302a/substrate/utils/substrate-bip39/src/lib.rs#L52-L70
  // https://wiki.polkadot.com/learn/learn-account-advanced/#portability
  static std::optional<std::array<uint8_t, kSr25519SeedSize>> MnemonicToSeed(
      std::string_view mnemonic,
      std::string_view password = "");

  PolkadotKeyring(base::span<const uint8_t, kSr25519SeedSize> seed,
                  mojom::KeyringId keyring_id);
  ~PolkadotKeyring();

  // Get address of the account denoted by `//<network>//<key_id>`, which is the
  // SS58-encoded public key for this particular derivation.
  // Polkadot has migrated to using ss58-prefix 0 for all account addresses
  // going forward, known as "unified addressing".
  std::string GetUnifiedAddress(uint32_t key_id);

  // Get the public key associated with the account denoted by
  // `//<network>//<key_id>`.
  std::array<uint8_t, kSr25519PublicKeySize> GetPublicKey(uint32_t key_id);

  // Use the derived account `key_id` to sign the provided message.
  std::array<uint8_t, kSr25519SignatureSize> SignMessage(
      base::span<const uint8_t> message,
      uint32_t key_id);

  // Verify that the provided signature is associated with the given message,
  // for the account denoted by `key_id`.
  [[nodiscard]] bool VerifyMessage(
      base::span<const uint8_t, kSr25519SignatureSize> signature,
      base::span<const uint8_t> message,
      uint32_t key_id);

  // Helper that tells us if this keyring is intended for the `//polkadot`
  // mainnet or the `//westend` testnet.
  bool IsTestNet() const noexcept;

 private:
  HDKeySr25519& GetKeypairOrInsert(uint32_t key_id);

  HDKeySr25519 root_account_key_;
  mojom::KeyringId keyring_id_;
  base::flat_map<uint32_t, HDKeySr25519> secondary_keys_;
};
}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_KEYRING_H_
