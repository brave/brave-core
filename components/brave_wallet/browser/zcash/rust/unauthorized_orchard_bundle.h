// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_UNAUTHORIZED_ORCHARD_BUNDLE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_UNAUTHORIZED_ORCHARD_BUNDLE_H_

#include <array>
#include <optional>

#include "base/containers/span.h"
#include "brave/components/brave_wallet/browser/zcash/rust/authorized_orchard_bundle.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"

namespace brave_wallet::orchard {

class UnauthorizedOrchardBundle {
 public:
  virtual ~UnauthorizedOrchardBundle() = default;

  static std::unique_ptr<UnauthorizedOrchardBundle> Create(
      base::span<const uint8_t> tree_state,
      const std::vector<::brave_wallet::OrchardOutput> orchard_outputs,
      std::optional<size_t> random_seed_for_testing);

  virtual std::array<uint8_t, kZCashDigestSize> GetDigest() = 0;
  virtual std::unique_ptr<AuthorizedOrchardBundle> Complete(
      const std::array<uint8_t, kZCashDigestSize>& sighash) = 0;
};

}  // namespace brave_wallet::orchard

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_UNAUTHORIZED_ORCHARD_BUNDLE_H_
