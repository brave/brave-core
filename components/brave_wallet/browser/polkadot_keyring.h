/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_KEYRING_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_KEYRING_H_

#include <optional>
#include <string_view>

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
                  mojom::KeyringId);
  ~PolkadotKeyring();
};
}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_KEYRING_H_
