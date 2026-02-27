/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_CHAIN_METADATA_PREFS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_CHAIN_METADATA_PREFS_H_

#include <optional>
#include <string_view>

#include "base/memory/raw_ref.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_chain_metadata.h"

class PrefService;

namespace brave_wallet {

class PolkadotChainMetadataPrefs {
 public:
  explicit PolkadotChainMetadataPrefs(PrefService& profile_prefs);
  virtual ~PolkadotChainMetadataPrefs();

  virtual std::optional<PolkadotChainMetadata> GetChainMetadata(
      std::string_view chain_id) const;
  virtual bool SetChainMetadata(std::string_view chain_id,
                                const PolkadotChainMetadata& metadata);

 private:
  const raw_ref<PrefService> profile_prefs_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_CHAIN_METADATA_PREFS_H_
