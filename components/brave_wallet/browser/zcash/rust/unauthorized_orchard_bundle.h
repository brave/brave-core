/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_UNAUTHORIZED_ORCHARD_BUNDLE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_UNAUTHORIZED_ORCHARD_BUNDLE_H_

#include <array>
#include <memory>
#include <optional>
#include <vector>

#include "base/containers/span.h"
#include "brave/components/brave_wallet/browser/zcash/rust/authorized_orchard_bundle.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"

namespace brave_wallet::orchard {

// UnauthorizedOrchardBundle represents input data needed to create
// Orchard part for Zcash transaction.
// Like anchor tree state(which is used for shielded inputs wittness
// calculation), random number generator, shielded inputs and shielded outputs.
class UnauthorizedOrchardBundle {
 public:
  virtual ~UnauthorizedOrchardBundle() = default;

  // Creates UnauthorizedOrchardBundle without shielded inputs
  static std::unique_ptr<UnauthorizedOrchardBundle> Create(
      base::span<const uint8_t> tree_state,
      const std::vector<::brave_wallet::OrchardOutput>& orchard_outputs,
      std::optional<size_t> random_seed_for_testing);

  // Before Complete is called we need to calculate signature digest which
  // combines all zcash transaction data.
  // This digest is used in
  // https://zips.z.cash/zip-0244#signature-digest
  virtual std::array<uint8_t, kZCashDigestSize> GetDigest() = 0;

  // On this step zero knowledge proof based on provided inputs is created and
  // signature is applied.
  // Reference in the zcash_primitives crate:
  // https://github.com/zcash/librustzcash/blob/5bd911f63bb9b41f97e4b37c32e79b52a7706543/zcash_primitives/src/transaction/builder.rs#L802
  // Note: this is CPU heavy method, should be executed on background thread.
  virtual std::unique_ptr<AuthorizedOrchardBundle> Complete(
      const std::array<uint8_t, kZCashDigestSize>& sighash) = 0;
};

}  // namespace brave_wallet::orchard

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_UNAUTHORIZED_ORCHARD_BUNDLE_H_
