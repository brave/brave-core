/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_SUBSTRATE_KEYRING_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_SUBSTRATE_KEYRING_H_

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/containers/span.h"
#include "brave/components/brave_wallet/browser/internal/hd_key_ed25519.h"
#include "brave/components/brave_wallet/browser/secp256k1_hd_keyring.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"

namespace brave_wallet {

// Keyring for the Polkadot ecosystem.
// Currently supports only ed25519 cryptography and a single account.
class PolkadotSubstrateKeyring {
 public:
  explicit PolkadotSubstrateKeyring(base::span<const uint8_t> seed);
  ~PolkadotSubstrateKeyring();
  PolkadotSubstrateKeyring(const PolkadotSubstrateKeyring&) = delete;
  PolkadotSubstrateKeyring& operator=(const PolkadotSubstrateKeyring&) = delete;

  std::optional<std::string> GetAccountAddress(uint16_t network_prefix, size_t index);

  std::vector<uint8_t> SignMessage(const std::string& address,
                                   base::span<const uint8_t> message);

  std::optional<std::string> AddNewHDAccount(size_t index);

  mojom::KeyringId keyring_id() const { return mojom::KeyringId::kPolkadotSubstrateMainnet; }

 private:
  std::optional<std::string> GetSS58AddressString(uint8_t network_prefix, const HDKeyEd25519& key);

  std::unique_ptr<HDKeyEd25519> root_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_SUBSTRATE_KEYRING_H_
