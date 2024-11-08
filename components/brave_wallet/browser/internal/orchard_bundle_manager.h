// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_ORCHARD_BUNDLE_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_ORCHARD_BUNDLE_MANAGER_H_

#include <array>
#include <memory>
#include <optional>
#include <vector>

#include "brave/components/brave_wallet/browser/zcash/rust/authorized_orchard_bundle.h"
#include "brave/components/brave_wallet/browser/zcash/rust/unauthorized_orchard_bundle.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"

namespace brave_wallet {

// This class manages orchard bundle unauthorized and authorized states
// Initially state is unauthorized, to convert it to authorized state you need
// to call ApplySignature with proper sighash.
// See also orchard crate's Bundle:
// https://github.com/zcash/orchard/blob/2b9c9a1deb66f8b20cd5c07fdd0cec87895f5c16/src/bundle.rs
class OrchardBundleManager {
 public:
  ~OrchardBundleManager();

  // Orchard digest is used to construct tx signature digest see
  // https://zips.z.cash/zip-0244
  // Called on unauthorized state
  std::optional<std::array<uint8_t, kZCashDigestSize>> GetOrchardDigest();

  // Apply tx signature digest switch to authorized state
  // Called on unauthorized state
  std::unique_ptr<OrchardBundleManager> ApplySignature(
      std::array<uint8_t, kZCashDigestSize> sighash);

  // Returns raw orchard bytes to use in transaction
  // If state is unauthorized returns nullopt.
  // Called on authorized state
  std::optional<std::vector<uint8_t>> GetRawTxBytes();

  // Creates instance for shielded outputs only
  // Returns in unauthorized state
  static std::unique_ptr<OrchardBundleManager> Create(
      base::span<const uint8_t> tree_state,
      const OrchardSpendsBundle& spends_bundle,
      const std::vector<OrchardOutput>& orchard_outputs);

  static void OverrideRandomSeedForTesting(size_t seed) {
    random_seed_for_testing_ = seed;
  }

 private:
  explicit OrchardBundleManager(
      std::unique_ptr<orchard::UnauthorizedOrchardBundle> unauthorized_bundle);
  explicit OrchardBundleManager(
      std::unique_ptr<orchard::AuthorizedOrchardBundle> authorized_bundle);

  static std::optional<size_t> random_seed_for_testing_;

  std::unique_ptr<orchard::UnauthorizedOrchardBundle>
      unauthorized_orchard_bundle_;
  std::unique_ptr<orchard::AuthorizedOrchardBundle> authorized_orchard_bundle_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_ORCHARD_BUNDLE_MANAGER_H_
